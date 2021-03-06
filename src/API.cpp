#include "API.h"

API *API::api = new API();

API::API() {
    auto im = new IndexManager;
    auto bm = new BufferManager;
    rm = new RecordManager(bm, im);
    cm = new CatalogManager;

    // rebuild indices
    for (auto &index : cm->GetIndices())
        rm->BuildIndex(cm->GetTableByName(index.table), cm->GetAttrByName(index.table, index.attribute));
}

API::~API() {
    delete rm->bm;
    delete rm->im;
    delete rm;
    delete cm;
}

bool API::Exit() {
    delete api;
    return true;
}

bool API::Execute(MINI_TYPE::SqlCommand sqlCommand) {
    using namespace MINI_TYPE;
    switch (sqlCommand.commandType) {
        case CreateTableCmd:
            return CreateTable(sqlCommand.tableInfo);
            break;
        case DropTableCmd:
            return DropTable(sqlCommand.tableName);
            break;
        case CreateIndexCmd:
//          return CreateIndex(sqlCommand.indexInfo);
            break;
        case DropIndexCmd:
            return DropIndex(sqlCommand.indexName);
            break;
        case SelectCmd:
            return Select(sqlCommand.tableName,
                          sqlCommand.condArray,
                          sqlCommand.attrList);
            break;
        case InsertCmd:
            return Insert(sqlCommand.tableName,
                          sqlCommand.valueArray);
            break;
        case DeleteCmd:
            return Delete(sqlCommand.tableName,
                          sqlCommand.condArray);
            break;
        default:
            // DO NOTHING
            break;
    }
    return false;
}

bool API::CreateTable(MINI_TYPE::TableInfo tableInfo) {

    // 1) check if the table exists

    if (api->cm->TableExists(tableInfo.name)) {
        std::cerr << "Table " << tableInfo.name << " exists." << std::endl;
        return false;
    }

    // 2) check if the parameters are valid

//    if (tableInfo.primaryKey.empty()) {
//        std::cerr << "No primary key." << std::endl;
//        return false;
//    }

    for (auto &attr : tableInfo.attributes)
        if (attr.type.type == MINI_TYPE::TypeId::MiniChar && !MINI_TYPE::IsValidString(attr.type.char_size)) {
            std::cerr << "String out of range." << std::endl;
            return false;
        }

    // 3) start creating table

    if (!tableInfo.primaryKey.empty())
        api->cm->MakeAttrUniqueAndPrimary(tableInfo, tableInfo.primaryKey);
    api->cm->CreateTable(tableInfo);
    api->rm->CreateTableFile(tableInfo);
    for (auto &attr : tableInfo.attributes) {
        if (attr.unique) {
            api->cm->CreateIndex(tableInfo.name, attr.name);
            api->rm->BuildIndex(tableInfo, api->cm->GetAttrByName(tableInfo, attr.name));
        }
    }
    api->cm->SaveCatalogToFile();

    return true;
}

bool API::CreateIndex(const MINI_TYPE::IndexInfo indexInfo) {

    // 1) check if the index exists

    if (api->cm->IndexExists(indexInfo.name)) {
        std::cerr << "Index " << indexInfo.name << " exists." << std::endl;
        return false;
    }

    // 2) check if the table exists

    if (!api->cm->TableExists(indexInfo.table)) {
        std::cerr << "Table " << indexInfo.table << " does not exist." << std::endl;
        return false;
    }

    // 3) check if the attribute is unique
    auto &attr = api->cm->GetAttrByName(indexInfo.table, indexInfo.attribute);
    if (!attr.unique) {
        std::cerr << "Attribute " << attr.name << " is not unique." << std::endl;
        return false;
    }

    // 4) start creating index
    api->cm->CreateIndex(indexInfo);
    api->cm->AttachIndexToTable(indexInfo);

    auto &tableInfo = api->cm->GetTableByName(indexInfo.table);
    api->rm->BuildIndex(tableInfo, api->cm->GetAttrByName(tableInfo, indexInfo.attribute));
    api->cm->SaveCatalogToFile();

    return true;
}

bool API::DropTable(const std::string tableName) {

    // 1) check if the table exists

    if (!api->cm->TableExists(tableName)) {
        std::cerr << "Table " << tableName << " does not exist." << std::endl;
        return false;
    }

    // 2) start deleting
    auto &tableInfo = api->cm->GetTableByName(tableName);
    auto indexInfos = api->cm->GetIndexInfoConcerned(tableInfo);
    for (auto &indexInfo : indexInfos)
        api->rm->DropIndex(tableInfo, api->cm->GetAttrByName(tableInfo, indexInfo.attribute));
    api->rm->DeleteTableFile(tableInfo);
    api->cm->DeleteTable(tableName); // includes DeleteIndex
    api->cm->SaveCatalogToFile();

    return true;
}

bool API::DropIndex(std::string indexName) {

    // 1) check if the index exists

    auto indexInfo = api->cm->IndexFindAndNormalizeAlias(indexName);
    if (indexInfo.table == "NULL") {
        std::cerr << "Index " << indexName << " does not exist." << std::endl;
        return false;
    }

    // 2) start deleting
    auto &tableInfo = api->cm->GetTableByName(indexInfo.table);
    api->rm->DropIndex(
            tableInfo,
            api->cm->GetAttrByName(tableInfo, indexInfo.attribute)
    );
    api->cm->DeleteIndex(indexName);
    api->cm->SaveCatalogToFile();

    return true;
}

bool API::Delete(const std::string tableName, std::vector<MINI_TYPE::Condition> condList) {

    // 1) check if the table exists

    if (!api->cm->TableExists(tableName)) {
        std::cerr << "Table " << tableName << " does not exist." << std::endl;
        return false;
    }

    // 2) start deleting
    for (auto &cond : condList)
        cond.value.type.type = api->cm->GetAttrTypeByName(tableName, cond.attributeName);

    api->rm->DeleteRecord(api->cm->GetTableByName(tableName), condList);

    return true;
}

bool API::Select(const std::string tableName, std::vector<MINI_TYPE::Condition> condList,
                 std::vector<std::string> attrList) {

    if (attrList.empty())
        attrList = api->cm->GetAttrNames(tableName);

    // 1) check if the table exists

    if (!api->cm->TableExists(tableName)) {
        std::cerr << "Table " << tableName << " does not exist." << std::endl;
        return false;
    }

    // 2) start selecting
    MINI_TYPE::Table tableRes;
    std::vector<std::string> attrWithIndexList;

    bool existNeq = false;
    for (auto &cond : condList) {
        if (cond.op == MINI_TYPE::Operator::NotEqual)
            existNeq = true;
        cond.value.type.type = api->cm->GetAttrTypeByName(tableName, cond.attributeName);
        if (api->cm->IndexExists(MINI_TYPE::IndexName(tableName, cond.attributeName)))
            attrWithIndexList.push_back(cond.attributeName);
    }

    auto &tableInfo = api->cm->GetTableByName(tableName);
    if (existNeq || attrWithIndexList.empty())
        tableRes = api->rm->SelectRecord(tableInfo, condList);
    else
        tableRes = api->rm->SelectRecord(tableInfo, condList,
                                         attrWithIndexList[0]);

    if (tableRes.records.empty())
        std::cout << "Not Found." << std::endl;
    else
        tableRes.DisplayAttr(attrList);

    return true;
}

bool API::Insert(std::string tableName, std::vector<MINI_TYPE::SqlValue> valueList) {
    // 1) check if the table exists

    if (!api->cm->TableExists(tableName)) {
        std::cerr << "Table " << tableName << " does not exist." << std::endl;
        return false;
    }

    // 2) check if the value types are valid

    auto &tableInfo = api->cm->GetTableByName(tableName);
    if (tableInfo.attributes.size() != valueList.size()) {
        std::cerr << "Invalid size of value list." << std::endl;
        return false;
    }

    for (int i = 0; i < tableInfo.attributes.size(); i++) {

//        if (tableInfo.attributes[i].type != valueList[i].type) {
//            std::cerr << "Type mismatch." << std::endl;
//            return false;
//        }

        valueList[i].type = tableInfo.attributes[i].type;

        if (valueList[i].type.type == MINI_TYPE::TypeId::MiniChar &&
            !MINI_TYPE::IsValidString(valueList[i].type.char_size)) {
            std::cerr << "Invalid string." << std::endl;
            return false;
        }

        if (tableInfo.attributes[i].unique)
            if (api->rm->im->Find(MINI_TYPE::IndexName(tableInfo.name, tableInfo.attributes[i].name), valueList[i]) !=
                IndexManager::end) {
                std::cerr << "Attribute " << tableInfo.attributes[i].name << " should be unique." << std::endl;
                return false;
            }

    }

    // 3) start inserting

    MINI_TYPE::Record record(valueList);
    api->rm->InsertRecord(tableInfo, record);

    return true;
}
