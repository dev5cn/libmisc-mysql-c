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

#include <mysql.h>
#include "MysqlResultRow.h"

MysqlResultRow::MysqlResultRow(shared_ptr<unordered_map<string, int>> col)
{
	this->row = NULL;
	this->col = col;
}

MySqlCrudRow* MysqlResultRow::mutableRow()
{
	if (this->row == NULL)
		this->row = new MySqlCrudRow();
	return this->row;
}

bool MysqlResultRow::getBool(int col, bool& val)
{
	if ((int) this->row->col.size() <= col)
	{
		LOG_ERROR("over the column size: %zu, col: %d", this->row->col.size(), col)
		return false;
	}
	val = this->row->col.at(col)->valLong == 0; 
	return true;
}

bool MysqlResultRow::getBool(const string& field, bool& val)
{
	auto it = this->col->find(field);
	if (it == this->col->end())
	{
		LOG_ERROR("can not found filed in result: %s", field.c_str())
		return false;
	}
	return this->getBool(it->second, val);
}

bool MysqlResultRow::getInt(int col, uint& val)
{
	ullong x;
	bool ret = this->getLong(col, x);
	val = (uint) x;
	return ret;
}

bool MysqlResultRow::getInt(const string& field, uint& val)
{
	ullong x;
	bool ret = this->getLong(field, x);
	val = (uint) x;
	return ret;
}

bool MysqlResultRow::getLong(int col, ullong& val)
{
	if ((int) this->row->col.size() <= col)
	{
		LOG_ERROR("over the column size: %zu, col: %d", this->row->col.size(), col)
		return false;
	}
	val = this->row->col.at(col)->valLong;
	return true;
}

bool MysqlResultRow::getLong(const string& field, ullong& val)
{
	auto it = this->col->find(field);
	if (it == this->col->end())
	{
		LOG_ERROR("can not found filed in result: %s", field.c_str())
		return false;
	}
	return this->getLong(it->second, val);
}

bool MysqlResultRow::getStr(int col, string& val)
{
	if ((int) this->row->col.size() <= col)
	{
		LOG_ERROR("over the column size: %zu, col: %d", this->row->col.size(), col)
		return false;
	}
	val = this->row->col.at(col)->valBin;
	return true;
}

bool MysqlResultRow::getStr(const string& field, string& val)
{
	auto it = this->col->find(field);
	if (it == this->col->end())
	{
		LOG_ERROR("can not found filed in result: %s", field.c_str())
		return false;
	}
	return this->getStr(it->second, val);
}

bool MysqlResultRow::getBin(int col, string& val)
{
	return this->getStr(col, val);
}

bool MysqlResultRow::getBin(const string& field, string& val)
{
	return this->getStr(field, val);
}

string MysqlResultRow::yyyy_mm_dd_hh_mi_ss(int col)
{
	ullong val;
	return this->getLong(col, val) ? DateMisc::to_yyyy_mm_dd_hh_mi_ss(val / 1000L) : "";
}

string MysqlResultRow::yyyy_mm_dd_hh_mi_ss(const string& field)
{
	auto it = this->col->find(field);
	if (it == this->col->end())
	{
		LOG_ERROR("can not found filed in result: %s", field.c_str())
		return "";
	}
	return this->yyyy_mm_dd_hh_mi_ss(it->second);
}

string MysqlResultRow::yyyy_mm_dd_hh_mi_ss_ms(int col)
{
	ullong val;
	return this->getLong(col, val) ? DateMisc::to_yyyy_mm_dd_hh_mi_ss_ms(val) : "";
}

string MysqlResultRow::yyyy_mm_dd_hh_mi_ss_ms(const string& field)
{
	auto it = this->col->find(field);
	if (it == this->col->end())
	{
		LOG_ERROR("can not found filed in result: %s", field.c_str())
		return "";
	}
	return this->yyyy_mm_dd_hh_mi_ss_ms(it->second);
}

const string& MysqlResultRow::columnName(int indx)
{
	static string str = "";
	for (auto& it : *this->col)
	{
		if (it.second == indx)
			return it.first;
	}
	return str;
}

string MysqlResultRow::toString()
{
	string str;
	SPRINTF_STRING(&str, "{ ")
	for (size_t i = 0; i < this->row->col.size(); ++i)
	{
		mysql_crud_field* f = this->row->col.at(i);
		string name = this->columnName(i);
		if (f->type == MYSQL_TYPE_VARCHAR)
		{
			SPRINTF_STRING(&str, "\"%s\" : \"%s\"%s", name.c_str(), f->valBin.c_str(), (i == this->row->col.size() - 1) ? "" : ", ")
			continue;
		}
		if (f->type == MYSQL_TYPE_LONG)
		{
			SPRINTF_STRING(&str, "\"%s\" : %llu%s", name.c_str(), f->valLong, (i == this->row->col.size() - 1) ? "" : ", ")
			continue;
		}
		if (f->type == MYSQL_TYPE_DATETIME)
		{
			SPRINTF_STRING(&str, "\"%s\" : \"%s\"%s", name.c_str(), DateMisc::to_yyyy_mm_dd_hh_mi_ss_ms(f->valLong).c_str(), (i == this->row->col.size() - 1) ? "" : ", ")
			continue;
		}
		if (f->type == MYSQL_TYPE_BLOB)
		{
			SPRINTF_STRING(&str, "\"%s\" : \"%s\"%s", name.c_str(), Net::hex2strUperCase((uchar* )f->valBin.data(), f->valBin.length()).c_str(), (i == this->row->col.size() - 1) ? "" : ", ")
			continue;
		}
		LOG_FAULT("it`s a bug, unexpected field type: %d", f->type)
	}
	SPRINTF_STRING(&str, " }")
	return str;
}

MysqlResultRow::~MysqlResultRow()
{
	delete this->row;
}

