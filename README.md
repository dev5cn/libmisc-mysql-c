# libmisc-mysql-c

### how to build?

* mkdir build;cd build

* git clone https://github.com/dev5cn/libmisc-cpp

* git clone https://github.com/dev5cn/libmisc-mysql-c

* cd libmisc-mysql-c

* export MYSQLC_DRIVER=/home/dev5/tools/mysql-community-devel-8.0.17-1/

* export CXX_FLAGS="-g3 -O3"

* chmod 775 build.sh clean.sh

* ./build.sh

* done.


### how to use?

* create the connection pool.

```js
MysqlConnPool::instance()->init("host", 3306, "x_msg_im", "root", "password", 0x08);
```

* insert one or batch.

```js
MYSQL* conn = MysqlConnPool::instance()->getConn();
string sql;
SPRINTF_STRING(&sql, "insert into %s values (?, ?, ?, ?, ?, ?)", "tb_x_msg_im")
shared_ptr<MysqlCrudReq> req(new MysqlCrudReq(sql));
req->addRow() //
->addVarchar("cgt") //
->addBool(true) //
->addBlob("dat") //
->addLong(1ULL) //
->addDateTime(DateMisc::nowGmt0()) //
->addDateTime(DateMisc::nowGmt0());
bool ret = MysqlMisc::sql((MYSQL*) conn, req, [](int ret, const string& desc, int effected)
{
    if (ret != 0)
    {
        LOG_ERROR("insert into tb_x_msg_im failed, ret: %04X, error: %s", ret, desc.c_str())
        return;
    }
    LOG_TRACE("insert into tb_x_msg_im successful, effected: %d", effected)
});
MysqlConnPool::instance()->relConn(conn, ret);
```

* select 

```js
MYSQL* conn = MysqlConnPool::instance()->getConn();
string sql;
SPRINTF_STRING(&sql, "select * from %s",  "tb_x_msg_im")
bool ret = MysqlMisc::query(conn, sql, [](int ret, const string& desc, bool more, int rows, shared_ptr<MysqlResultRow> row)
{
    if (ret != 0) 
    {
        LOG_ERROR("load tb_x_msg_im failed, ret: %d, desc: %s", ret, desc.c_str())
        return false;
    }
    if (row == NULL) 
    {
        LOG_DEBUG("table tb_x_msg_im no record")
        return true;
    }
    string cgt;
    row->getStr("cgt", cgt);
    bool enable;
    row->getBool("enable", enable);
    string dat;
    row->getBlob("dat", dat);
    ullong ver;
    row->getLong("ver", ver);
    ullong gts;
    row->getLong("gts", gts);
    ullong uts;
    row->getLong("uts", uts);
    return true;
});
MysqlConnPool::instance()->relConn(conn, ret);
```
