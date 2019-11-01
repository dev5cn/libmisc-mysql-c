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

#include "MysqlConnPool.h"

MysqlConnPool* MysqlConnPool::inst = new MysqlConnPool();

MysqlConnPool::MysqlConnPool()
{

}

MysqlConnPool* MysqlConnPool::instance()
{
	return MysqlConnPool::inst;
}

bool MysqlConnPool::init(const string& host, const int port, const string& db, const string& usr, const string& pwd, const int poolSize)
{
	this->host = host;
	this->db = db;
	this->port = port;
	this->usr = usr;
	this->pwd = pwd;
	this->poolSize = poolSize < 1 ? 1 : (poolSize > 0x40 ? 0x40 : poolSize);
	for (int i = 0; i < this->poolSize; ++i)
	{
		MYSQL* conn = this->getConnReal();
		if (conn == NULL)
			return false;
		this->pool.push_back(conn);
	}
	this->aliveSize = poolSize;
	return true;
}

string MysqlConnPool::getDbName()
{
	return this->db;
}

MYSQL* MysqlConnPool::getConn()
{
	unique_lock<mutex> lock(this->lock4pool);
	if (!this->pool.empty()) 
	{
		MYSQL* conn = this->pool.back();
		this->pool.pop_back();
		return conn;
	}
	if (this->aliveSize >= this->poolSize) 
	{
		LOG_ERROR("can not allocate idle mysql connection")
		return NULL;
	}
	MYSQL* conn = this->getConnReal();
	if (conn == NULL)
		return NULL;
	++this->aliveSize;
	return conn;
}

void MysqlConnPool::relConn(MYSQL* conn, bool normal)
{
	unique_lock<mutex> lock(this->lock4pool);
	if (normal)
	{
		this->pool.push_front(conn);
		return;
	}
	::mysql_close(conn);
	::free(conn);
	--this->aliveSize;
}

MYSQL* MysqlConnPool::getConnReal()
{
	MYSQL* conn = (MYSQL*) calloc(1, sizeof(MYSQL));
	if (::mysql_init(conn) == NULL)
	{
		LOG_ERROR("init mysql connection failed")
		::free(conn);
		return NULL;
	}
	if (::mysql_real_connect(conn, this->host.c_str(), this->usr.c_str(), this->pwd.c_str(), this->db.c_str(), this->port, NULL, CLIENT_MULTI_STATEMENTS) != NULL)
	{
		if (::mysql_set_character_set(conn, "utf8mb4") != 0)
		{
			LOG_ERROR("can not set mysql-character to utf8mb4")
			::mysql_close(conn);
			::free(conn);
			return NULL;
		}
		char reConnect = 1;
		int connTimeOut = 15;
		int readTimeOut = 15;
		int writeTimeOut = 15;
		::mysql_options(conn, MYSQL_OPT_RECONNECT, (char*) &reConnect);
		::mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, (const char*) &connTimeOut);
		::mysql_options(conn, MYSQL_OPT_READ_TIMEOUT, (const char*) &readTimeOut);
		::mysql_options(conn, MYSQL_OPT_WRITE_TIMEOUT, (const char*) &writeTimeOut);
		LOG_INFO("got a connection from mysql-%s database, db: %s, character: %s, thread-id: %lu", conn->server_version, conn->db, conn->options.charset_name, ::mysql_thread_id(conn))
		return conn;
	}
	LOG_ERROR("can not connect to mysql database, host: %s:%d", this->host.c_str(), this->port)
	::free(conn);
	return NULL;
}

MysqlConnPool::~MysqlConnPool()
{

}

