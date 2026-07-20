//-----------------------------------------------------------------------------
// 文件: inventory_controller.cpp
// 描述: Drogon 控制器实现 — 请求解析与响应格式化
//-----------------------------------------------------------------------------

#include "inventory_controller.h"

#include <drogon/HttpResponse.h>
#include <json/json.h>

#include <sstream>

#include "concurrent_test.h"
#include "consistency_checker.h"
#include "log.h"

namespace demo1
{

InventoryController::InventoryController(std::shared_ptr<IInventoryService> service)
    : m_service(std::move(service))
{
}

// ============================================================
// 响应辅助方法
// ============================================================

drogon::HttpResponsePtr InventoryController::make_ok(const std::string& data)
{
    Json::Value root;
    root["code"] = 0;
    root["msg"]  = "success";
    if (!data.empty())
    {
        Json::Reader reader;
        Json::Value  data_val;
        if (reader.parse(data, data_val))
        {
            root["data"] = data_val;
        }
        else
        {
            // 非 JSON 字符串，直接作为 data 字段
            root["data"] = data;
        }
    }
    auto resp = drogon::HttpResponse::newHttpJsonResponse(root);
    resp->setStatusCode(drogon::k200OK);
    return resp;
}

drogon::HttpResponsePtr InventoryController::make_error(int code, const std::string& msg)
{
    Json::Value root;
    root["code"] = code;
    root["msg"]  = msg;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(root);
    resp->setStatusCode(code < 0 ? drogon::k400BadRequest : drogon::k200OK);
    return resp;
}

// ============================================================
// POST /api/v1/deduct
// ============================================================

void InventoryController::deduct(const drogon::HttpRequestPtr& req,
                                  std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto json = req->getJsonObject();
    if (!json)
    {
        callback(make_error(-1, "Invalid JSON body"));
        return;
    }

    int64_t item_id  = (*json)["item_id"].asInt64();
    int     quantity = (*json)["quantity"].asInt();

    if (item_id <= 0 || quantity <= 0)
    {
        callback(make_error(-1, "Invalid parameters: item_id and quantity required"));
        return;
    }

    DeductRequest dreq;
    dreq.item_id  = item_id;
    dreq.quantity = quantity;

    auto result = m_service->deduct(dreq);
    if (!result)
    {
        const auto& err = result.unwrap_err();
        LOGW("Deduct failed: item_id=%lld, qty=%d, error=%s",
             static_cast<long long>(item_id), quantity, err.to_string().c_str());

        if (err.code() == kStockInsufficient)
        {
            callback(make_error(-1, "Insufficient stock"));
            return;
        }
        callback(make_error(-500, err.to_string()));
        return;
    }

    auto& resp_val = *result;
    Json::Value data;
    data["remaining_stock"] = resp_val.remaining_stock;

    Json::Value root;
    root["code"] = 0;
    root["msg"]  = "success";
    root["data"] = data;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(root);
    callback(resp);
}

// ============================================================
// POST /api/v1/test/concurrent
// ============================================================

void InventoryController::concurrent_test(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback)
{
    auto json = req->getJsonObject();

    int64_t item_id     = 1;
    int     quantity    = 1;
    int     concurrency = 100;

    if (json)
    {
        item_id     = (*json).get("item_id", 1).asInt64();
        quantity    = (*json).get("quantity", 1).asInt();
        concurrency = (*json).get("concurrency", 100).asInt();
    }

    ConcurrentTestRequest test_req;
    test_req.item_id     = item_id;
    test_req.quantity    = quantity;
    test_req.concurrency = concurrency;

    ConcurrentTestService test_service(*m_service);
    auto result = test_service.run(test_req);

    if (!result)
    {
        callback(make_error(-500, result.unwrap_err().to_string()));
        return;
    }

    auto& r = *result;
    Json::Value data;
    data["total_requests"]    = r.total_requests;
    data["success"]           = r.success_count;
    data["fail"]              = r.fail_count;
    data["theoretical_remain"] = r.theoretical_remain;
    data["actual_redis_stock"] = r.actual_redis_stock;
    data["actual_mysql_stock"] = r.actual_mysql_stock;
    data["data_consistent"]   = r.data_consistent;
    data["summary"]           = r.summary;

    Json::Value root;
    root["code"] = 0;
    root["msg"]  = "success";
    root["data"] = data;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(root);
    callback(resp);
}

// ============================================================
// GET /api/v1/check/{item_id}
// ============================================================

void InventoryController::check(const drogon::HttpRequestPtr& /*req*/,
                                 std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                                 int64_t item_id)
{
    ConsistencyChecker checker(*m_service);
    auto result = checker.check(item_id);

    if (!result)
    {
        callback(make_error(-500, result.unwrap_err().to_string()));
        return;
    }

    auto& r = *result;
    Json::Value data;
    data["item_id"]     = static_cast<Json::Int64>(r.item_id);
    data["mysql_stock"] = r.mysql_stock;
    data["consistent"]  = r.consistent;

    if (r.redis_available)
    {
        data["redis_stock"] = r.redis_stock;
    }
    else
    {
        data["redis_stock"] = Json::Value::null;
    }

    if (!r.suggestion.empty())
    {
        data["suggestion"] = r.suggestion;
    }

    Json::Value root;
    root["code"] = 0;
    root["msg"]  = "success";
    root["data"] = data;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(root);
    callback(resp);
}

}  // namespace demo1