# 技术验证 Demo：电商库存扣减系统（Redis + MySQL）

## 1. 系统架构概览

本 Demo 采用 **“Redis 优先扣减，MySQL 事务持久化”** 的读写策略。

- **读写路径**：读请求优先命中 Redis，写请求先原子扣减 Redis，再同步（或异步）更新 MySQL。
- **一致性保障**：通过 Lua 脚本保证 Redis 层原子性，通过数据库事务保证 MySQL 层原子性，并通过补偿机制处理异常。
- **降级策略**：Redis 故障时，自动熔断至 MySQL 悲观锁扣减（牺牲性能保可用性）。

---

## 2. 全局配置文件定义 (`config.ini`)

所有环境依赖集中管理，便于切换测试场景。

```yaml
server:
  port: 8080

redis:
  host: 127.0.0.1
  port: 6379
  password: ""
  database: 0
  pool:
    max_active: 50        # 最大活跃连接数
    max_idle: 10          # 最大空闲连接数
    idle_timeout: 300s    # 连接超时时间

mysql:
  dsn: "root:password@tcp(127.0.0.1:3306)/inventory_db?charset=utf8mb4&parseTime=True&loc=Local"
  max_open_conns: 20
  max_idle_conns: 5
  conn_max_lifetime: 60s

business:
  stock_key_prefix: "stock:"    # Redis Key 前缀
  default_timeout: 5s           # 业务超时时间
  retry_times: 3                # 补偿重试次数
```

---

## 3. 模块详细设计

### 步骤 1：初始化基础数据 (Bootstrap)

**功能描述**：应用启动时自动检查并初始化 1000 件商品（ID: 1~1000），每件初始库存为 10000。

- **输入**：无（自动触发）。
- **输出**：日志记录初始化结果。
- **关键逻辑（伪代码）**：

```pseudo
function InitInventory():
    for item_id = 1 to 1000:
        // 1. MySQL 幂等插入（存在则忽略）
        sql = "INSERT INTO inventory (item_id, stock) VALUES (?, 10000) ON DUPLICATE KEY UPDATE stock=stock"
        db.Exec(sql, item_id)
        
        // 2. 同步 Redis（覆盖写入，确保一致性）
        key = "stock:" + item_id
        redis.Set(key, 10000, 0)  // 0 表示永不过期（由业务逻辑控制）
        log.Info("Init item: ", item_id)
    log.Info("Init completed. Total items: 1000")
```

- **数据库 DDL**（开发者自行准备）：
```sql
CREATE TABLE `inventory` (
  `item_id` bigint(20) NOT NULL PRIMARY KEY,
  `stock` int(11) NOT NULL DEFAULT 0,
  `version` int(11) DEFAULT 0 COMMENT '乐观锁字段(可选)',
  `updated_at` datetime DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);
```

---

### 步骤 2：核心功能——库存扣减 (Deduct)

**功能描述**：提供 HTTP 接口 `POST /api/v1/deduct`，实现安全的库存扣减。

- **输入**：JSON 格式 `{"item_id": 1001, "quantity": 5}`。
- **输出**：`{"code": 0, "msg": "success", "data": {"remaining_stock": 9995}}`。
- **关键逻辑（含原子 Lua 脚本）**：

```pseudo
// Lua 脚本：原子性检查并扣减
lua_script = """
local key = KEYS[1]
local deduct_qty = tonumber(ARGV[1])
local current = redis.call('GET', key)

if not current then
    return {-1, 0}  -- Key 不存在（理论上初始化过）
end

current = tonumber(current)
if current < deduct_qty then
    return {0, current}  -- 库存不足
end

local new_stock = redis.call('DECRBY', key, deduct_qty)
return {1, new_stock}  -- 扣减成功
"""

function DeductStock(item_id, quantity):
    key = "stock:" + item_id
    
    // 1. 执行 Lua 脚本（原子操作）
    result, new_stock = redis.Eval(lua_script, [key], [quantity])
    
    // 2. 处理扣减结果
    if result == -1:
        // 降级处理：尝试从 MySQL 重建缓存（见步骤4）
        return fallback_to_mysql(item_id, quantity)
    elif result == 0:
        return {code: -1, msg: "Insufficient stock", current: new_stock}
    else:
        // 3. ★ 扣减成功 → 同步更新 MySQL（必须事务）
        mysql_tx = db.Begin()
        try:
            // 使用悲观锁（FOR UPDATE）防止并发更新丢失，确保数据最终一致
            sql_update = "UPDATE inventory SET stock = stock - ? WHERE item_id = ? AND stock - ? >= 0"
            rows_affected = mysql_tx.Exec(sql_update, quantity, item_id, quantity)
            if rows_affected == 0:
                // 理论上不可能发生（Redis已扣），若发生则触发补偿回滚
                raise Exception("MySQL update conflict")
            
            mysql_tx.Commit()
            log.Info("Deduct success", "item", item_id, "qty", quantity, "remain", new_stock)
            return {code: 0, msg: "success", data: {remaining_stock: new_stock}}
        catch (Exception e):
            mysql_tx.Rollback()
            // ★ 关键补偿：MySQL 更新失败，回滚 Redis 已扣数量
            redis.IncrBy(key, quantity)
            log.Error("MySQL tx failed, Redis rolled back", "error", e)
            return {code: -500, msg: "System error, please retry"}
```

---

### 步骤 3：并发安全验证 (Concurrency Testing)

**功能描述**：提供批量下单模拟接口 `POST /api/v1/test/concurrent`，启动 100 个 Goroutine/线程同时扣减指定商品。

- **输入**：`{"item_id": 1, "quantity": 1, "concurrency": 100}`。
- **输出**：返回成功/失败次数统计，以及最终库存校验结果。
- **关键逻辑（伪代码）**：

```pseudo
function ConcurrentDeductTest(item_id, qty, concurrency=100):
    var wg sync.WaitGroup
    success_count = 0
    fail_count = 0
    errors = []  // 记录异常详情
    
    for i = 0; i < concurrency; i++:
        wg.Add(1)
        go function():
            defer wg.Done()
            resp = call_api("/deduct", {item_id, qty})
            if resp.code == 0:
                atomic.Add(&success_count, 1)
            else:
                atomic.Add(&fail_count, 1)
                if resp.msg == "Insufficient stock":
                    errors.append("Stock exhausted at round " + i)
    
    wg.Wait()
    
    // 验证最终一致性
    final_redis = redis.Get("stock:" + item_id)
    final_mysql = db.Query("SELECT stock FROM inventory WHERE item_id=?", item_id)
    
    return {
        "total_requests": concurrency,
        "success": success_count,
        "fail": fail_count,
        "theoretical_remain": 10000 - (success_count * qty),
        "actual_redis_stock": final_redis,
        "actual_mysql_stock": final_mysql,
        "data_consistent": (final_redis == final_mysql == theoretical_remain)
    }
```

---

### 步骤 4：异常处理机制 (Fault Tolerance)

#### 4.1 Redis 不可用时降级
- **触发条件**：Redis 连接超时或 `PING` 失败。
- **降级逻辑（伪代码）**：

```pseudo
function fallback_to_mysql(item_id, quantity):
    log.Warn("Redis unavailable, degrading to MySQL pessimistic lock")
    mysql_tx = db.Begin()
    // 使用 SELECT ... FOR UPDATE 锁定行，阻止其他并发扣减
    row = mysql_tx.QueryRow("SELECT stock FROM inventory WHERE item_id = ? FOR UPDATE", item_id)
    if row.stock < quantity:
        mysql_tx.Rollback()
        return {code: -1, msg: "Insufficient stock"}
    
    new_stock = row.stock - quantity
    mysql_tx.Exec("UPDATE inventory SET stock = ? WHERE item_id = ?", new_stock, item_id)
    mysql_tx.Commit()
    
    // 尝试异步重建 Redis（如果恢复）
    async_try_rebuild_redis(item_id, new_stock)
    return {code: 0, data: {remaining_stock: new_stock}}
```

#### 4.2 库存不足快速失败（防击穿）
- **逻辑**：若 Redis 返回值 `current` 为 0 或负数，**直接返回**失败，不查询 MySQL（避免热点 Key 穿透数据库）。
- **伪代码**：在 Lua 脚本返回 `{0, current}` 时，立即响应，不触发 MySQL 读取。

#### 4.3 MySQL 更新失败补偿机制
- **场景**：Redis DECRBY 成功，但 MySQL UPDATE 因为网络抖动或死锁超时失败。
- **实现**（已集成于步骤 2）：
  1. 捕获异常回滚 MySQL 事务。
  2. 立即执行 `redis.IncrBy(key, quantity)` 回滚 Redis。
  3. 记录详细错误日志至 `compensation_log` 表（异步落盘），供人工介入重试。

---

### 步骤 5：数据一致性验证 (Consistency Audit)

**功能描述**：提供查询接口 `GET /api/v1/check/{item_id}`，实时对比 Redis 和 MySQL 数据。

- **输入**：Path 参数 `item_id`。
- **输出**：JSON 对比报告。
- **关键逻辑（伪代码）**：

```pseudo
function CheckConsistency(item_id):
    redis_stock, err = redis.Get("stock:" + item_id)
    if err:
        redis_stock = "NULL (connection error)"
    
    mysql_row = db.QueryRow("SELECT stock, updated_at FROM inventory WHERE item_id = ?", item_id)
    mysql_stock = mysql_row.stock
    
    is_match = (redis_stock == mysql_stock)
    
    return {
        "item_id": item_id,
        "redis_stock": redis_stock,
        "mysql_stock": mysql_stock,
        "consistent": is_match,
        "suggestion": if !is_match: "Check compensation logs or manual repair needed"
    }
```

---

## 4. 验收标准验证步骤

| 验收条目 | 具体验证操作 | 预期结果 |
| :--- | :--- | :--- |
| **1. 扣减接口返回状态** | 使用 `curl -X POST ... -d '{"item_id":1,"quantity":10}'` | 返回 HTTP 200，`code=0`，且 `remaining_stock` 正确减少 10。 |
| **2. 并发 100 零误差** | 执行并发测试接口，传入 `concurrency=100, quantity=1`。 | 返回结果中 `actual_redis_stock == actual_mysql_stock == 9900`（初始 10000 - 100）。成功数 + 失败数 = 100，且无负数库存。 |
| **3. Redis 宕机降级** | 手动停止 Redis 服务，执行单次扣减。 | 日志输出 `Degrading to MySQL`，接口依然返回成功，且 MySQL 库存正确减少。恢复 Redis 后，数据自动补齐（或手动补齐）。 |
| **4. 查询接口对比** | 执行多次扣减后，调用 `/check/1`。 | `consistent` 字段为 `true`。若为 `false`，检查日志中的补偿记录。 |
| **5. 日志追踪清晰** | 查看运行日志，搜索 `Deduct success` 或 `compensation`。 | 每笔扣减包含 `item_id`、`qty`、`remain`、耗时等结构化字段，链路完整。 |

---

## 5. 测试建议（开发者自测指南）

### 测试 1：功能测试（单次扣减）
```bash
# 扣减 1 件
curl -X POST http://localhost:8080/api/v1/deduct -H "Content-Type: application/json" -d '{"item_id": 1, "quantity": 1}'
# 预期: {"code":0, "data":{"remaining_stock":9999}}
# 再次查询一致性
curl http://localhost:8080/api/v1/check/1
# 预期: {"consistent": true, "redis_stock": 9999, "mysql_stock": 9999}
```

### 测试 2：压力测试（并发 100）
```bash
curl -X POST http://localhost:8080/api/v1/test/concurrent -H "Content-Type: application/json" -d '{"item_id": 1, "quantity": 1, "concurrency": 100}'
# 观察输出统计：总成功数应等于 100（若初始库存充足），最终库存 = 初始 - 100。
```

### 测试 3：降级测试（Redis 故障）
1. 执行 `redis-cli SHUTDOWN`。
2. 发送扣减请求。
3. 查看日志包含 `Redis unavailable`，接口返回成功。
4. 重启 Redis，执行 `GET stock:1`，若为旧值，手动调用初始化接口或等待 TTL 过期重建（Demo 中可直接调用 `/admin/rebuild/{id}` 触发重建）。

### 测试 4：一致性测试（边界场景）
- 模拟 MySQL 超时：在代码中给 MySQL UPDATE 添加 `SELECT SLEEP(10)` 人为制造超时，观察日志是否触发 `Redis rolled back`，随后手动检查 Redis 值是否恢复至扣减前。

---

## 6. 开发者编码注意事项

1.  **连接池管理**：务必在应用启动时初始化全局 `redis.Pool` 和 `db.Pool`，所有操作复用指针，严禁 `new` 连接。
2.  **超时控制**：所有网络 IO（Redis GET/SET、MySQL Query/Exec）必须设置 Context 超时（如 3s），防止协程泄漏。
3.  **日志规范**：建议使用结构化日志（如 `zap` 或 `logrus`），包含 `trace_id` 以便串联单次请求的全链路（Redis 扣减 -> MySQL 更新）。
4.  **补偿日志表**（扩展建议）：
```sql
CREATE TABLE `compensation_log` (
  `id` bigint AUTO_INCREMENT PRIMARY KEY,
  `item_id` bigint NOT NULL,
  `quantity` int NOT NULL,
  `redis_new_stock` int,
  `error_msg` varchar(255),
  `status` tinyint DEFAULT 0 COMMENT '0-pending,1-fixed',
  `created_at` datetime DEFAULT CURRENT_TIMESTAMP
);
```

---
