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
#include "MySqlCrudRow.h"

MySqlCrudRow::MySqlCrudRow()
{

}

MySqlCrudRow* MySqlCrudRow::addBool(bool val)
{
	return this->addLong(val ? 0 : 1);
}

MySqlCrudRow* MySqlCrudRow::addLong(ullong val)
{
	mysql_crud_field* field = new mysql_crud_field();
	field->type = MYSQL_TYPE_LONG;
	field->valLong = val;
	this->col.push_back(field);
	return this;
}

MySqlCrudRow* MySqlCrudRow::addVarchar(const string& val)
{
	mysql_crud_field* field = new mysql_crud_field();
	field->type = MYSQL_TYPE_VARCHAR;
	field->valBin = val;
	this->col.push_back(field);
	return this;
}

MySqlCrudRow* MySqlCrudRow::addDateTime(ullong val)
{
	mysql_crud_field* field = new mysql_crud_field();
	field->type = MYSQL_TYPE_DATETIME;
	field->valLong = val;
	this->col.push_back(field);
	return this;
}

MySqlCrudRow* MySqlCrudRow::addBlob(const string& val)
{
	mysql_crud_field* field = new mysql_crud_field();
	field->type = MYSQL_TYPE_BLOB;
	field->valBin = val;
	this->col.push_back(field);
	return this;
}

MySqlCrudRow::~MySqlCrudRow()
{
	for (size_t i = 0; i < this->col.size(); ++i)
		delete this->col[i];
}
