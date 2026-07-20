//-----------------------------------------------------------------------------
// 文件: inventory_controller.h
// 描述: Drogon HTTP 控制器 — 请求解析与响应格式化
//       继承 drogon::HttpController，通过 METHOD_LIST_BEGIN 声明路由
//-----------------------------------------------------------------------------

#pragma once

#include <drogon/HttpController.h>
#include <drogon/HttpTypes.h>

#include <memory>

#include "inventory_service.h"

namespace demo1
{

/// @brief 库存控制器 — Drogon HttpController 实现
/// 负责请求解析、参数校验、响应格式化，不包含业务逻辑
class InventoryController : public drogon::HttpController<InventoryController>
{
public:
    /// @param service 库存业务服务（shared_ptr，与 App 共享生命周期）
    explicit InventoryController(std::shared_ptr<IInventoryService> service);

    METHOD_LIST_BEGIN

    /// POST /api/v1/deduct — 库存扣减
    ADD_METHOD_TO(InventoryController::deduct, "/api/v1/deduct", drogon::Post);

    /// POST /api/v1/test/concurrent — 并发压测
    ADD_METHOD_TO(InventoryController::concurrent_test, "/api/v1/test/concurrent", drogon::Post);

    /// GET /api/v1/check/{item_id} — 数据一致性检查
    ADD_METHOD_TO(InventoryController::check, "/api/v1/check/{item_id}", drogon::Get);

    METHOD_LIST_END

private:
    /// @brief 扣减库存
    void deduct(const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /// @brief 并发测试
    void concurrent_test(const drogon::HttpRequestPtr& req,
                         std::function<void(const drogon::HttpResponsePtr&)>&& callback);

    /// @brief 一致性检查
    void check(const drogon::HttpRequestPtr& req,
               std::function<void(const drogon::HttpResponsePtr&)>&& callback,
               int64_t item_id);

    /// @brief 构建 JSON 成功响应
    static drogon::HttpResponsePtr make_ok(const std::string& data);

    /// @brief 构建 JSON 错误响应
    static drogon::HttpResponsePtr make_error(int code, const std::string& msg);

private:
    std::shared_ptr<IInventoryService> m_service;
};

}  // namespace demo1