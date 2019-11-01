/*
  Copyright 2019 www.dev5.cn, Inc. dev5@qq.com
 
  This file is part of X-MSG-IM.
 
  X-MSG-IM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  X-MSG-IM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU Affero General Public License
  along with X-MSG-IM.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef MYSQLMISC_H_
#define MYSQLMISC_H_

#include <mysql.h>
#include "MysqlCrudReq.h"
#include "MysqlResultRow.h"

class MysqlMisc
{
public:
	static bool sql(MYSQL* conn, const string& sql, int& ret, string& desc, int* affected = NULL ); 
	static bool sql(MYSQL* conn, shared_ptr<MysqlCrudReq> req, int& ret, string& desc, int* affected = NULL ); 
	static bool sql(MYSQL* conn, const string& sql, function<void(int ret, const string& desc, int affected)> cb); 
	static bool sql(MYSQL* conn, shared_ptr<MysqlCrudReq> req, function<void(int ret, const string& desc, int affected)> cb); 
	static bool query(MYSQL* conn, const string& sql, function<bool(int ret, const string& desc, bool more, int rows, shared_ptr<MysqlResultRow> row)> cb); 
	static bool query(MYSQL* conn, shared_ptr<MysqlCrudReq> req, function<bool(int ret, const string& desc, bool more, int rows, shared_ptr<MysqlResultRow> row)> cb); 
	static bool start(MYSQL* conn); 
	static bool commit(MYSQL* conn); 
	static bool rollBack(MYSQL* conn); 
public:
	static bool longVal(MYSQL* conn, const string& sql, ullong& val); 
	static bool intVal(MYSQL* conn, const string& sql, uint& val); 
private:
	static bool execBindOneNoResult(MYSQL* conn, shared_ptr<MysqlCrudReq> req, int& ret, string& desc, int* affected = NULL ); 
	static bool execBindBatchNoResult(MYSQL* conn, shared_ptr<MysqlCrudReq> req, int& ret, string& desc, int* affected = NULL ); 
	static bool execNoResult(MYSQL* conn, const string& sql, int& ret, string& desc, int* affected = NULL ); 
	static bool bindAndExe(MYSQL_STMT* stmt, const MySqlCrudRow* row, int& ret, string& desc); 
	static bool takeResult(MYSQL_STMT* stmt, const string& sql, function<bool(int ret, const string& desc, bool more, int rows, shared_ptr<MysqlResultRow> row)> cb); 
	static bool fetchRow(MYSQL_STMT* stmt, MySqlCrudRow* row, vector<pair<enum_field_types, string>>& types, int& ret, string& desc, unsigned long* len, MYSQL_BIND* bind); 
	MysqlMisc();
	virtual ~MysqlMisc();
};

#endif 
