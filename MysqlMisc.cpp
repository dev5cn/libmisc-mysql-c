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

#include "MysqlMisc.h"
#include "MysqlConnPool.h"

MysqlMisc::MysqlMisc()
{

}

bool MysqlMisc::sql(MYSQL* conn, const string& sql, int& ret, string& desc, int* affected)
{
	return MysqlMisc::execNoResult(conn, sql, ret, desc, affected);
}

bool MysqlMisc::sql(MYSQL* conn, shared_ptr<MysqlCrudReq> req, int& ret, string& desc, int* affected)
{
	return (req->row.empty()) ? MysqlMisc::execNoResult(conn, req->sql, ret, desc, affected) : 
								((req->row.size() == 1) ? MysqlMisc::execBindOneNoResult(conn, req, ret, desc, affected) : 
															MysqlMisc::execBindBatchNoResult(conn, req, ret, desc, affected));
}

bool MysqlMisc::sql(MYSQL* conn, const string& sql, function<void(int ret, const string& desc, int affected)> cb)
{
	int ret = 0;
	string desc;
	int affected = 0;
	bool r = MysqlMisc::execNoResult(conn, sql, ret, desc, &affected);
	cb(r ? 0 : (ret == 0 ? 1 : ret), desc, affected);
	return r;
}

bool MysqlMisc::sql(MYSQL* conn, shared_ptr<MysqlCrudReq> req, function<void(int ret, const string& desc, int affected)> cb)
{
	int ret = 0;
	string desc;
	int affected = 0;
	bool r = MysqlMisc::sql(conn, req, ret, desc, &affected);
	cb(r ? 0 : (ret == 0 ? 1 : ret), desc, affected);
	return r;
}

bool MysqlMisc::query(MYSQL* conn, const string& sql, function<bool(int ret, const string& desc, bool more, int rows, shared_ptr<MysqlResultRow> row)> cb)
{
	MYSQL_STMT* stmt = ::mysql_stmt_init(conn);
	if (stmt == NULL)
	{
		int ret = ::mysql_stmt_errno(stmt);
		string desc = ::mysql_stmt_error(stmt);
		LOG_ERROR("mysql_stmt_init failed, err: %s, sql: %s", desc.c_str(), sql.c_str())
		cb(ret, desc, false, 0, nullptr);
		return false;
	}
	if (::mysql_stmt_prepare(stmt, sql.c_str(), sql.length()) != 0)
	{
		int ret = ::mysql_stmt_errno(stmt);
		string desc = ::mysql_stmt_error(stmt);
		::mysql_stmt_close(stmt);
		LOG_ERROR("mysql_stmt_prepare failed, err: %s, sql: %s", desc.c_str(), sql.c_str())
		cb(ret, desc, false, 0, nullptr);
		return false;
	}
	if (::mysql_stmt_execute(stmt))
	{
		int ret = ::mysql_stmt_errno(stmt);
		string desc = ::mysql_stmt_error(stmt);
		::mysql_stmt_close(stmt);
		LOG_ERROR("mysql_stmt_execute failed, err: %s, sql: %s", desc.c_str(), sql.c_str())
		cb(ret, desc, false, 0, nullptr);
		return false;
	}
	return MysqlMisc::takeResult(stmt, sql, cb);
}

bool MysqlMisc::query(MYSQL* conn, shared_ptr<MysqlCrudReq> req, function<bool(int ret, const string& desc, bool more, int rows, shared_ptr<MysqlResultRow> row)> cb)
{
	if (req->row.empty())
	{
		LOG_ERROR("must be have parameter bind, req: %s", req->toString().c_str())
		cb(1, "must be have parameter bind", false, 0, nullptr);
		return false;
	}
	MYSQL_STMT* stmt = ::mysql_stmt_init(conn);
	if (stmt == NULL)
	{
		int ret = ::mysql_errno(conn);
		string desc = ::mysql_error(conn);
		LOG_ERROR("mysql_stmt_init failed, err: %s, req: %s", desc.c_str(), req->toString().c_str())
		cb(ret, desc, false, 0, nullptr);
		return false;
	}
	if (::mysql_stmt_prepare(stmt, req->sql.c_str(), req->sql.length()))
	{
		int ret = ::mysql_stmt_errno(stmt);
		string desc = ::mysql_stmt_error(stmt);
		::mysql_stmt_close(stmt);
		LOG_ERROR("mysql_stmt_prepare failed, err: %s, sql: %s", desc.c_str(), req->toString().c_str())
		cb(ret, desc, false, 0, nullptr);
		return false;
	}
	int ret;
	string desc;
	if (!MysqlMisc::bindAndExe(stmt, req->row.at(0), ret, desc))
	{
		::mysql_stmt_close(stmt);
		LOG_ERROR("mysql bind parameters failed, may be have unexpected field type, sql: %s", req->toString().c_str())
		cb(ret, desc, false, 0, nullptr);
		return false;
	}
	bool r = MysqlMisc::takeResult(stmt, req->sql, cb);
	::mysql_stmt_close(stmt);
	return r;
}

bool MysqlMisc::takeResult(MYSQL_STMT* stmt, const string& sql, function<bool(int ret, const string& desc, bool more, int rows, shared_ptr<MysqlResultRow> row)> cb)
{
	MYSQL_RES* res = ::mysql_stmt_result_metadata(stmt);
	if (res == NULL)
	{
		int ret = ::mysql_stmt_errno(stmt);
		string desc = ::mysql_stmt_error(stmt);
		LOG_ERROR("mysql_stmt_store_result failed, err: %s, sql: %s", desc.c_str(), sql.c_str())
		cb(ret, desc, false, 0, nullptr);
		return false;
	}
	shared_ptr<unordered_map<string, int>> col(new unordered_map<string, int>());
	vector<pair<enum_field_types, string>> types;
	int indx = 0;
	MYSQL_FIELD* field = ::mysql_fetch_field(res);
	while (field != NULL)
	{
		types.push_back(make_pair<>(field->type, field->name));
		col->insert(make_pair<>(field->name, indx));
		++indx;
		field = ::mysql_fetch_field(res);
	}
	unsigned long* len = (unsigned long*) ::calloc(1, sizeof(unsigned long) * types.size());
	MYSQL_BIND* bind = (MYSQL_BIND*) ::calloc(1, sizeof(MYSQL_BIND) * types.size());
	for (size_t i = 0; i < types.size(); ++i)
	{
		bind[i].buffer = NULL;
		bind[i].buffer_length = 0;
		bind[i].length = len + i;
	}
	if (::mysql_stmt_bind_result(stmt, bind) != 0)
	{
		int ret = ::mysql_stmt_errno(stmt);
		string desc = ::mysql_stmt_error(stmt);
		::free(len);
		::free(bind);
		::mysql_free_result(res);
		LOG_ERROR("mysql_stmt_bind_result failed, err: %s, sql: %s", desc.c_str(), sql.c_str())
		cb(ret, desc, false, 0, nullptr);
		return false;
	}
	if (::mysql_stmt_store_result(stmt) != 0)
	{
		int ret = ::mysql_stmt_errno(stmt);
		string desc = ::mysql_stmt_error(stmt);
		::free(len);
		::free(bind);
		::mysql_free_result(res);
		LOG_ERROR("mysql_stmt_store_result failed, err: %s, sql: %s", desc.c_str(), sql.c_str())
		cb(ret, desc, false, 0, nullptr);
		return false;
	}
	ullong rows = ::mysql_stmt_num_rows(stmt);
	if (rows == 0) 
	{
		::free(len);
		::free(bind);
		::mysql_free_result(res);
		cb(0, "", false, 0, nullptr);
		return true;
	}
	int ret = 0;
	string desc;
	for (ullong i = 0; i < rows; ++i)
	{
		shared_ptr<MysqlResultRow> rst(new MysqlResultRow(col));
		if (MysqlMisc::fetchRow(stmt, rst->mutableRow(), types, ret, desc, len, bind))
		{
			if (!cb(ret, desc, i != rows - 1, rows, rst)) 
			{
				::free(len);
				::free(bind);
				::mysql_free_result(res);
				return false;
			}
			continue;
		}
		::free(len);
		::free(bind);
		::mysql_free_result(res);
		LOG_ERROR("fetch row failed, err: %s, sql: %s", desc.c_str(), sql.c_str())
		cb(ret, desc, false, 0, nullptr);
		return false;
	}
	::free(len);
	::free(bind);
	::mysql_free_result(res);
	return true;
}

bool MysqlMisc::fetchRow(MYSQL_STMT* stmt, MySqlCrudRow* row, vector<pair<enum_field_types, string>>& types, int& ret, string& desc, unsigned long* len, MYSQL_BIND* bind)
{
	int r = ::mysql_stmt_fetch(stmt);
	if (r != 0 && r != MYSQL_DATA_TRUNCATED && r != MYSQL_NO_DATA)
	{
		ret = ::mysql_stmt_errno(stmt);
		desc = ::mysql_stmt_error(stmt);
		LOG_ERROR("mysql_stmt_fetch failed, err: %s, r: %d", desc.c_str(), r)
		return false;
	}
	for (size_t i = 0; i < types.size(); ++i)
	{
		switch (types.at(i).first)
		{
		case MYSQL_TYPE_VAR_STRING:
		{
			if (len[i] < 1)
			{
				row->addVarchar("");
				continue;
			}
			bind[i].buffer_type = types.at(i).first;
			bind[i].buffer = ::malloc(len[i] + 1);
			bind[i].buffer_length = len[i];
			((char*) (bind[i].buffer))[bind[i].buffer_length] = 0x00;
			if (::mysql_stmt_fetch_column(stmt, bind + i, i, 0) != 0)
			{
				::free(bind[i].buffer);
				ret = ::mysql_stmt_errno(stmt);
				desc = ::mysql_stmt_error(stmt);
				LOG_ERROR("mysql_stmt_fetch_column failed, err: %s", desc.c_str())
				return false;
			}
			row->addVarchar((char*) bind[i].buffer);
			::free(bind[i].buffer);
		}
			break;
		case MYSQL_TYPE_LONGLONG:
		{
			ullong val = 0;
			bind[i].buffer_type = types.at(i).first;
			bind[i].buffer = &val;
			bind[i].buffer_length = sizeof(ullong);
			if (::mysql_stmt_fetch_column(stmt, bind + i, i, 0) != 0)
			{
				ret = ::mysql_stmt_errno(stmt);
				desc = ::mysql_stmt_error(stmt);
				LOG_ERROR("mysql_stmt_fetch_column failed, err: %s", desc.c_str())
				return false;
			}
			row->addLong(val);
		}
			break;
		case MYSQL_TYPE_BLOB:
		case MYSQL_TYPE_MEDIUM_BLOB:
		case MYSQL_TYPE_LONG_BLOB:
		{
			if (len[i] < 1)
			{
				row->addBlob("");
				continue;
			}
			bind[i].buffer_type = types.at(i).first;
			bind[i].buffer = ::malloc(len[i]);
			bind[i].buffer_length = len[i];
			if (::mysql_stmt_fetch_column(stmt, bind + i, i, 0) != 0)
			{
				::free(bind[i].buffer);
				ret = ::mysql_stmt_errno(stmt);
				desc = ::mysql_stmt_error(stmt);
				LOG_ERROR("mysql_stmt_fetch_column failed, err: %s", desc.c_str())
				return false;
			}
			row->addBlob(string((char*) bind[i].buffer, bind[i].buffer_length));
			::free(bind[i].buffer);
		}
			break;
		case MYSQL_TYPE_DATETIME:
		{
			MYSQL_TIME ts;
			bind[i].buffer_type = types.at(i).first;
			bind[i].buffer = &ts;
			bind[i].buffer_length = sizeof(MYSQL_TIME);
			if (::mysql_stmt_fetch_column(stmt, bind + i, i, 0) != 0)
			{
				ret = ::mysql_stmt_errno(stmt);
				desc = ::mysql_stmt_error(stmt);
				LOG_ERROR("mysql_stmt_fetch_column failed, err: %s", desc.c_str())
				return false;
			}
			struct tm ft;
			ft.tm_year = ts.year - 1900;
			ft.tm_mon = ts.month - 1;
			ft.tm_mday = ts.day;
			ft.tm_hour = ts.hour;
			ft.tm_min = ts.minute;
			ft.tm_sec = ts.second;
			time_t t = ::timegm(&ft);
			ullong uts = t;
			uts = uts * 1000 + ts.second_part / 1000;
			row->addDateTime(uts);
			break;
		}
		default:
		{
			LOG_FAULT("it`s a bug, unexpected field type: %d", types.at(i).first)
			ret = 0x01;
			desc = "unexpected field type";
			return false;
		}
			break;
		}
	}
	return true;
}

bool MysqlMisc::execBindOneNoResult(MYSQL* conn, shared_ptr<MysqlCrudReq> req, int& ret, string& desc, int* affected)
{
	ret = 0;
	bool r = false;
	MYSQL_STMT* stmt = ::mysql_stmt_init(conn);
	if (stmt == NULL)
	{
		ret = ::mysql_errno(conn);
		desc = ::mysql_error(conn);
		LOG_ERROR("mysql_stmt_init failed, err: %s, req: %s", desc.c_str(), req->toString().c_str())
		goto label;
	}
	if (::mysql_stmt_prepare(stmt, req->sql.c_str(), req->sql.length()))
	{
		ret = ::mysql_stmt_errno(stmt);
		desc = ::mysql_stmt_error(stmt);
		LOG_ERROR("mysql_stmt_prepare failed, err: %s, sql: %s", desc.c_str(), req->toString().c_str())
		goto label;
	}
	if (!MysqlMisc::bindAndExe(stmt, req->row.at(0), ret, desc))
	{
		LOG_ERROR("bind parameters or execute sql failed, sql: %s", req->toString().c_str())
		goto label;
	}
	r = true;
	if (affected != NULL)
		*affected = ::mysql_affected_rows(conn);
	label:
	::mysql_stmt_close(stmt);
	return r;
}

bool MysqlMisc::execBindBatchNoResult(MYSQL* conn, shared_ptr<MysqlCrudReq> req, int& ret, string& desc, int* affected)
{
	::mysql_autocommit(conn, 0);
	ret = 0;
	bool r = false;
	MYSQL_STMT* stmt = ::mysql_stmt_init(conn);
	if (stmt == NULL)
	{
		ret = ::mysql_errno(conn);
		desc = ::mysql_error(conn);
		LOG_ERROR("mysql_stmt_init failed, err: %s, req: %s", desc.c_str(), req->toString().c_str())
		goto label;
	}
	if (::mysql_stmt_prepare(stmt, req->sql.c_str(), req->sql.length()))
	{
		ret = ::mysql_stmt_errno(stmt);
		desc = ::mysql_stmt_error(stmt);
		LOG_ERROR("mysql_stmt_prepare failed, err: %s, sql: %s", desc.c_str(), req->toString().c_str())
		goto label;
	}
	for (size_t i = 0; i < req->row.size(); ++i)
	{
		if (!MysqlMisc::bindAndExe(stmt, req->row.at(i), ret, desc))
		{
			LOG_ERROR("mysql bind parameters failed, may be have unexpected field type, sql: %s", req->toString().c_str())
			goto label;
		}
		if (affected != NULL)
			*affected += ::mysql_affected_rows(conn);
		::mysql_stmt_reset(stmt);
	}
	r = true;
	label:
	::mysql_stmt_close(stmt);
	if (!r)
		::mysql_rollback(conn);
	else
		::mysql_commit(conn);
	::mysql_autocommit(conn, 1); 
	return r;
}

bool MysqlMisc::execNoResult(MYSQL* conn, const string& sql, int& ret, string& desc, int* affected)
{
	if (::mysql_real_query(conn, sql.c_str(), sql.length()) != 0)
	{
		ret = ::mysql_errno(conn);
		desc = ::mysql_error(conn);
		return false;
	}
	if (affected != NULL)
		*affected = ::mysql_affected_rows(conn);
	return true;
}

bool MysqlMisc::bindAndExe(MYSQL_STMT* stmt, const MySqlCrudRow* row, int& ret, string& desc)
{
	bool r = false;
	MYSQL_BIND* bind = (MYSQL_BIND*) ::calloc(1, sizeof(MYSQL_BIND) * row->col.size());
	shared_ptr<vector<MYSQL_TIME*>> datetimes = nullptr;
	for (size_t i = 0; i < row->col.size(); ++i)
	{
		mysql_crud_field* field = row->col[i];
		switch (field->type)
		{
		case MYSQL_TYPE_LONG:
		{
			bind[i].buffer_type = MYSQL_TYPE_LONG;
			bind[i].buffer = &(field->valLong);
		}
			break;
		case MYSQL_TYPE_VARCHAR:
		{
			bind[i].buffer_type = MYSQL_TYPE_VARCHAR;
			bind[i].buffer = (char*) field->valBin.c_str();
			bind[i].buffer_length = field->valBin.length();
		}
			break;
		case MYSQL_TYPE_BLOB:
		{
			bind[i].buffer_type = MYSQL_TYPE_MEDIUM_BLOB; 
			bind[i].buffer = field->valBin.data();
			bind[i].buffer_length = field->valBin.length();
		}
			break;
		case MYSQL_TYPE_DATETIME:
		{
			if (datetimes == nullptr)
				datetimes.reset(new vector<MYSQL_TIME*>());
			time_t tts = field->valLong / 1000L;
			struct tm ft;
			::localtime_r((time_t*) &tts, &ft);
			MYSQL_TIME* datetime = (MYSQL_TIME*) ::calloc(1, sizeof(MYSQL_TIME));
			datetime->year = ft.tm_year + 1900;
			datetime->month = ft.tm_mon + 1;
			datetime->day = ft.tm_mday;
			datetime->hour = ft.tm_hour;
			datetime->minute = ft.tm_min;
			datetime->second = ft.tm_sec;
			datetime->second_part = (field->valLong % 1000L) * 1000; 
			datetime->time_type = MYSQL_TIMESTAMP_DATETIME;
			datetimes->push_back(datetime);
			bind[i].buffer_type = MYSQL_TYPE_DATETIME;
			bind[i].buffer = datetime;
		}
			break;
		default:
		{
			LOG_FAULT("it`s a bug, unexpected field type: %d", field->type)
			ret = 0x01;
			desc = "unexpected field type";
			goto label;
		}
			break;
		}
	}
	if (::mysql_stmt_bind_param(stmt, bind) != 0) 
	{
		ret = ::mysql_stmt_errno(stmt);
		desc = ::mysql_stmt_error(stmt);
		LOG_ERROR("mysql_stmt_bind_param failed, err: %s", desc.c_str())
		goto label;
	}
	if (::mysql_stmt_execute(stmt) != 0)
	{
		ret = ::mysql_stmt_errno(stmt);
		desc = ::mysql_stmt_error(stmt);
		LOG_ERROR("mysql_stmt_execute failed, err: %s", desc.c_str())
		goto label;
	}
	r = true;
	label:
	::free(bind);
	if (datetimes != nullptr)
	{
		for (size_t i = 0; i < datetimes->size(); ++i)
			::free(datetimes->at(i));
	}
	return r;
}

bool MysqlMisc::start(MYSQL* conn)
{
	return ::mysql_autocommit(conn, 0) == 0;
}

bool MysqlMisc::commit(MYSQL* conn)
{
	if (::mysql_commit(conn) != 0)
	{
		::mysql_autocommit(conn, 1);
		return false;
	}
	::mysql_autocommit(conn, 1);
	return true;
}

bool MysqlMisc::rollBack(MYSQL* conn)
{
	if (::mysql_rollback(conn) != 0)
	{
		::mysql_autocommit(conn, 1);
		return false;
	}
	::mysql_autocommit(conn, 1);
	return true;
}

bool MysqlMisc::longVal(MYSQL* conn, const string& sql, ullong& val)
{
	return MysqlMisc::query(conn, sql, [&sql, &val](int ret, const string& desc, bool more, int rows, shared_ptr<MysqlResultRow> row)
	{
		if (ret != 0)
		{
			LOG_ERROR("execute sql failed: %s", sql.c_str())
			return false;
		}
		if (row == NULL) 
		{
			val = 0;
			return true;
		}
		if (!row->getLong(0, val))
		{
			LOG_ERROR("execute sql failed: %s", sql.c_str())
			return false;
		}
		return true;
	});
}

bool MysqlMisc::intVal(MYSQL* conn, const string& sql, uint& val)
{
	ullong x;
	if (!MysqlMisc::longVal(conn, sql, x))
		return false;
	val = (uint) x;
	return true;
}

MysqlMisc::~MysqlMisc()
{

}

