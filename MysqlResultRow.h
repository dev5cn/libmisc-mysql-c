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

#ifndef MYSQLRESULTROW_H_
#define MYSQLRESULTROW_H_

#include <libmisc.h>
#include "MySqlCrudRow.h"

class MysqlResultRow
{
public:
	bool getBool(int col, bool& val); 
	bool getBool(const string& field, bool& val); 
	bool getInt(int col, uint& val); 
	bool getInt(const string& field, uint& val); 
	bool getLong(int col, ullong& val); 
	bool getLong(const string& field, ullong& val); 
	bool getStr(int col, string& val); 
	bool getStr(const string& field, string& val); 
	bool getBin(int col, string& val); 
	bool getBin(const string& field, string& val); 
	string yyyy_mm_dd_hh_mi_ss(int col); 
	string yyyy_mm_dd_hh_mi_ss(const string& field); 
	string yyyy_mm_dd_hh_mi_ss_ms(int col); 
	string yyyy_mm_dd_hh_mi_ss_ms(const string& field); 
public:
	MySqlCrudRow* mutableRow();
	string toString();
	MysqlResultRow(shared_ptr<unordered_map<string, int>> col);
	virtual ~MysqlResultRow();
private:
	shared_ptr<unordered_map<string, int>> col;
	MySqlCrudRow* row;
	const string& columnName(int indx); 
};

typedef shared_ptr<MysqlResultRow> SptrRst;

#endif 
