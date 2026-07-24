-- ============================================================
-- Demo1: 电商库存扣减系统 — 数据库初始化脚本
-- 数据库: test_db
-- 描述: 创建库存表。应用启动时由 Bootstrap 负责填充初始数据
--       (1000 个商品, 每个初始库存 10000)
-- ============================================================

CREATE DATABASE IF NOT EXISTS `testdb`
  DEFAULT CHARACTER SET utf8mb4
  DEFAULT COLLATE utf8mb4_unicode_ci;

USE `testdb`;

CREATE TABLE IF NOT EXISTS `inventory` (
  `item_id`    BIGINT(20)  NOT NULL                           COMMENT '商品ID',
  `stock`      INT(11)     NOT NULL DEFAULT 0                 COMMENT '当前库存',
  `version`    INT(11)              DEFAULT 0                 COMMENT '乐观锁版本号(预留)',
  `updated_at` DATETIME             DEFAULT CURRENT_TIMESTAMP
                                    ON UPDATE CURRENT_TIMESTAMP COMMENT '最后更新时间',
  PRIMARY KEY (`item_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='库存表';