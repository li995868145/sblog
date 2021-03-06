#include "article_delete_servlet.h"
#include "sylar/log.h"
#include "blog/manager/article_manager.h"
#include "sylar/util.h"
#include "blog/my_module.h"
#include "sylar/email/smtp.h"
#include "sylar/env.h"
#include "sylar/sylar.h"
#include "blog/util.h"
#include "blog/struct.h"
#include <regex>

namespace blog {
namespace servlet {

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

ArticleDeleteServlet::ArticleDeleteServlet()
    :BlogLoginedServlet("ArticleDelete") {
}

int32_t ArticleDeleteServlet::handle(sylar::http::HttpRequest::ptr request
                                  ,sylar::http::HttpResponse::ptr response
                                  ,sylar::http::HttpSession::ptr session
                                  ,Result::ptr result) {
    do {
        DEFINE_AND_CHECK_STRING(result, ids, "ids");
        std::set<int64_t> art_ids;
        auto tmp = sylar::split(ids, ',');
        for(auto& i : tmp) {
            art_ids.insert(sylar::TypeUtil::Atoi(i));
        }
        int64_t uid = getUserId(request);
        if(!uid) {
            result->setResult(500, "not login");
            break;
        }

        std::vector<data::ArticleInfo::ptr> infos;
        for(auto& i : art_ids) {
            auto info = ArticleMgr::GetInstance()->get(i);
            if(info->getUserId() != uid) {
                continue;
            }
            if(info->getIsDeleted()) {
                continue;
            }
            infos.push_back(info);
        }

        auto db = getDB();
        if(!db) {
            result->setResult(500, "get db error");
            break;
        }
        auto trans = db->openTransaction();
        if(!trans) {
            result->setResult(500, "open transaction fail");
            break;
        }
        time_t now = time(0);
        for(auto& i : infos) {
            i->setIsDeleted(1);
            i->setUpdateTime(now);
            data::ArticleInfoDao::Update(i, db);
        }
        if(!trans->commit()) {
            SYLAR_LOG_ERROR(g_logger) << "commit fail";
            result->setResult(500, "commit fail");

            for(auto& i : infos) {
                i->setIsDeleted(0);
            }
            break;
        }
        result->setResult(200, "ok");
        if(!infos.empty()) {
            auto& jids = result->jsondata["ids"];
            for(auto& i : infos) {
                jids.append(i->getId());
            }
        }
    } while(false);
    
    response->setBody(result->toJsonString());
    return 0;
}

}
}
