#ifndef MINISQL_API_H
#define MINISQL_API_H

#include "MiniType.h"
#include "IndexManager.hpp"
#include "RecordManager.hpp"
#include "CatalogManager.h"

#include <string>

class API {
public:
    static bool CreateTable(MINI_TYPE::TableInfo tableInfo);

    static bool CreateIndex(MINI_TYPE::IndexInfo indexInfo);

    static bool DropTable(std::string tableName);

    static bool DropIndex(std::string indexName);

    static bool Delete(std::string tableName, std::vector<MINI_TYPE::Condition> condList);

    static bool Select(std::string tableName,
                       std::vector<MINI_TYPE::Condition> condList,
                       std::vector<MINI_TYPE::Attribute> attrList);

    static bool Insert(std::string tableName, std::vector<MINI_TYPE::SqlValue> valueList);

private:
    API();

    static API *api;

    RecordManager *rm;
    CatalogManager *cm;
};

#endif //MINISQL_API_H
