#include "API.h"

#include <cassert>

using namespace MINI_TYPE;

void CreateTableTest();
void DropTableTest();
void CreateIndexTest();
void DropIndexTest();
void SelectTest();
void InsertTest();
void DeleteTest();

int main() {

    CreateTableTest();
//    DropTableTest();
//    CreateIndexTest();
//    DropIndexTest();
//    SelectTest();
    InsertTest();
    SelectTest();
//    DeleteTest();

    API::Exit();

    return 0;
}

void CreateTableTest() {
//    create table student (
//            sno char(8),
//            sname char(16) unique,
//            sage int,
//            sgender char (1),
//            primary key ( sno )
//    );

    MINI_TYPE::TableInfo tableInfo;
    tableInfo.name = "student";
    SqlValueType t0(MiniChar, 8);
    SqlValueType t1(MiniChar, 16);
    SqlValueType t2(MiniInt);
    SqlValueType t3(MiniChar, 1);
    Attribute a0("sno", t0);
    Attribute a1("sname", t1, false, true);
    Attribute a2("sage", t2);
    Attribute a3("sgender", t3);
    tableInfo.attributes.push_back(a0);
    tableInfo.attributes.push_back(a1);
    tableInfo.attributes.push_back(a2);
    tableInfo.attributes.push_back(a3);
    tableInfo.primaryKey = "sno";

    API::CreateTable(tableInfo);
}

void DropTableTest(){
//    drop table student;

    API::DropTable("student");
}

void CreateIndexTest(){
//    create index stunameidx on student ( sname );

    CreateTableTest();

    IndexInfo indexInfo("student", "sname", "stunameidx");

    API::CreateIndex(indexInfo);
}

void DropIndexTest(){
//    drop index stunameidx;

    API::DropIndex("stunameidx");
}

void SelectTest(){
//    select * from student;
//    select * from student where sno = ‘88888888’;
//    select * from student where sage > 20 and sgender = ‘F’;

    std::vector<Condition> emptyCond;
    std::vector<std::string> emptyAttr;

    API::Select("student", emptyCond, emptyAttr);

    Condition c0;
    SqlValueType t0(MiniChar, 8);
    c0.value.type = t0;
    c0.value.str = "88888888";
    c0.op = Equal;
    c0.attributeName = "sno";
    std::vector<Condition> condList0 = {c0};

    API::Select("student", condList0, emptyAttr);

    Condition c1;
    Condition c2;
    SqlValueType t1(MiniInt);
    SqlValueType t2(MiniChar, 1);
    c1.value.type = t1;
    c1.value.i = 20;
    c1.op = GreaterThan;
    c1.attributeName = "sage";
    c2.value.type = t2;
    c2.value.str = "F";
    c2.op = Equal;
    c2.attributeName = "sgender";
    std::vector<Condition> condList1 = {c1, c2};

    API::Select("student", condList1, emptyAttr);
}

void InsertTest(){
//    insert into student values (‘12345678’,’wy’,22,’M’);

    SqlValueType t0(MiniChar, 8);
    SqlValueType t1(MiniChar, 2);
    SqlValueType t2(MiniInt);
    SqlValueType t3(MiniChar, 1);

    SqlValue v0(t0, "12345678");
    SqlValue v1(t1, "wy");
    SqlValue v2(t2, 22);
    SqlValue v3(t3, "M");

    std::vector<SqlValue> vec = {v0, v1, v2, v3};

    API::Insert("student", vec);
}

void DeleteTest(){
//    delete from student;
//    delete from student where sno = ‘88888888’;

    std::vector<Condition> condListEmpty;
    API::Delete("student", condListEmpty);

    Condition c0;
    c0.op = Equal;
    c0.attributeName = "sno";
    SqlValueType t0(MiniChar, 8);
    c0.value.type = t0;
    c0.value.str = "88888888";
    std::vector<Condition> condList = {c0};
    API::Delete("student", condList);
}