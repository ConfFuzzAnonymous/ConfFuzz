#include "../include/validate.h"
#include "../include/ast.h"
#include "../include/define.h"
#include "../include/utils.h"

#include <algorithm>
#include <climits>
#include <cfloat>
#include <malloc.h>

using namespace std;


vector<string> g_table_names;
vector<string> g_view_names;
vector<string> g_trigger_names;
map<string, vector<string>> g_table_column_info;
map<string, vector<string>> g_table_constraint_info;
map<string, vector<string>> g_table_index_info;

vector<string> t_table_names_add;
vector<string> t_table_names_del;
vector<string> t_view_names_add;
vector<string> t_view_names_del;
vector<string> t_trigger_names_add;
vector<string> t_trigger_names_del;
map<string, vector<string>> t_table_column_info_add;
map<string, vector<string>> t_table_column_info_del;
map<string, vector<string>> t_table_constraint_info_add;
map<string, vector<string>> t_table_constraint_info_del;
map<string, vector<string>> t_table_index_info_add;
map<string, vector<string>> t_table_index_info_del;


vector<s_column *> g_all_column;
vector<s_table *> g_all_table;
vector<certain_table *> g_all_certain_table;
graph g_stmt_graph;
set<IR *> g_used_table;               

vector<string> function_name_library = {
	"CUME_DIST", "DENSE_RANK", "FIRST_VALUE", "LAG", "LAST_VALUE", "LEAD", "NTH_VALUE", "NTILE", "PERCENT_RANK", "RANK", "ROW_NUMBER",
	"AVG", "BIT_AND", "BIT_OR", "BIT_XOR", "COUNT", "JSON_ARRAYAGG", "JSON_OBJECTAGG", "MAX", "MIN", "STDDEV_POP", "STDDEV", "STD", "STDDEV_SAMP", "SUM", "VAR_POP", "VARIANCE", "VAR_SAMP"};

vector<unsigned long> value_library = {0, (unsigned long)LONG_MAX, (unsigned long)ULONG_MAX,
									   (unsigned long)CHAR_BIT, (unsigned long)SCHAR_MIN, (unsigned long)SCHAR_MAX, (unsigned long)UCHAR_MAX,
									   (unsigned long)CHAR_MIN, (unsigned long)CHAR_MAX, (unsigned long)MB_LEN_MAX, (unsigned long)SHRT_MIN,
									   (unsigned long)INT_MIN, (unsigned long)INT_MAX, (unsigned long)SCHAR_MIN, (unsigned long)SCHAR_MIN,
									   (unsigned long)UINT_MAX, (unsigned long)FLT_MAX, (unsigned long)DBL_MAX, (unsigned long)LDBL_MAX,
									   (unsigned long)FLT_MIN, (unsigned long)DBL_MIN, (unsigned long)LDBL_MIN};

bool g_return_val;

void apply_changes(IRTYPE ir_type)
{

	switch (ir_type)
	{
	case kCreateStmt:
		
		if (t_table_names_add.size() != 0)
		{
			string cur_table_name = t_table_names_add[0];
			
			g_table_names.push_back(cur_table_name);
			for (auto cur_column_name : t_table_column_info_add[cur_table_name])
			{
				g_table_column_info[cur_table_name].push_back(cur_column_name);
			}

			for (auto cur_constraint_symbol : t_table_constraint_info_add[cur_table_name])
			{
				g_table_constraint_info[cur_table_name].push_back(cur_constraint_symbol);
			}
			return;
		}
		
		if (t_view_names_add.size() != 0)
		{
			string cur_view_name = t_view_names_add[0];
			
			g_table_column_info[cur_view_name].insert(g_table_column_info[cur_view_name].end(), t_table_column_info_add[cur_view_name].begin(), t_table_column_info_add[cur_view_name].end());
			return;
		}
		
		if (t_trigger_names_add.size() != 0)
		{
			string cur_trigger_name = t_trigger_names_add[0];
			
			g_trigger_names.push_back(cur_trigger_name);
			return;
		}
		
		if (t_table_index_info_add.size() != 0)
		{
			for (auto &p : t_table_index_info_add)
			{
				string cur_table_name = p.first;
				string cur_index_name = p.second[0];
				g_table_index_info[cur_table_name].push_back(cur_index_name);
				
			}
			return;
		}
	case kDropStmt:
		if (t_table_index_info_del.size() != 0)
		{ 
			for (auto &p : t_table_index_info_del)
			{
				string cur_table_name = p.first;
				string cur_index_name = p.second[0];
				g_table_index_info[cur_table_name].erase(remove(g_table_index_info[cur_table_name].begin(), g_table_index_info[cur_table_name].end(), cur_index_name), g_table_index_info[cur_table_name].end());
				if (g_table_index_info[cur_table_name].size() == 0)
				{
					g_table_index_info.erase(cur_table_name);
				}
			}
			return;
		}

		if (t_table_names_del.size() != 0)
		{ 
			string cur_table_name = t_table_names_del[0];
			g_table_names.erase(remove(g_table_names.begin(), g_table_names.end(), cur_table_name), g_table_names.end());
			g_table_column_info.erase(cur_table_name);
			g_table_constraint_info.erase(cur_table_name);
			g_table_index_info.erase(cur_table_name);
			return;
		}

		if (t_trigger_names_del.size() != 0)
		{ 
			string cur_trigger_name = t_trigger_names_del[0];
			g_trigger_names.erase(remove(g_trigger_names.begin(), g_trigger_names.end(), cur_trigger_name), g_trigger_names.end());
			return;
		}

		if (t_view_names_del.size() != 0)
		{ 
			string cur_view_name = t_view_names_del[0];
			g_view_names.erase(remove(g_view_names.begin(), g_view_names.end(), cur_view_name), g_view_names.end());
			g_table_column_info.erase(cur_view_name);
			return;
		}

	case kAlterStmt:
		if (!(t_table_names_add.size() == 1) || !(t_table_names_del.size() == 1))
			return;
		if (t_table_names_add[0] != t_table_names_del[0]) 
		{
			string new_table_name = t_table_names_add[0];
			string old_table_name = t_table_names_del[0];
			g_table_names.push_back(new_table_name);
			g_table_names.erase(remove(g_table_names.begin(), g_table_names.end(), old_table_name), g_table_names.end());

			g_table_column_info[new_table_name].insert(g_table_column_info[new_table_name].end(), g_table_column_info[old_table_name].begin(), g_table_column_info[old_table_name].end());
			g_table_column_info.erase(old_table_name);
			return;
		}
		if (t_table_names_add[0] == t_table_names_del[0]) 
		{
			string table_name = t_table_names_del[0];
			for (auto column : t_table_column_info_del[table_name])
			{
				g_table_column_info[table_name].erase(remove(g_table_column_info[table_name].begin(), g_table_column_info[table_name].end(), column), g_table_column_info[table_name].end());
			}
			g_table_column_info[table_name].insert(g_table_column_info[table_name].end(), t_table_column_info_add[table_name].begin(), t_table_column_info_add[table_name].end());
			return;
		}
	default:
		break;
	}
}



#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <sstream>
#include <regex>
#include <set>

set<string> create_tables;
set<string> drop_tables;
set<string> create_views;
set<string> drop_views;

void writeLog_table()
{
	writeLog("new_ir_tree.log", "=============================");
	writeLog("new_ir_tree.log", "all table:");
	string data;
	for (auto iter : g_table_column_info)
	{
		data = iter.first + ":";
		for (auto column_name : iter.second)
		{
			data += column_name + ", ";
		}
		writeLog("new_ir_tree.log", data.c_str());
	}

	writeLog("new_ir_tree.log", "all index:");
	for (auto iter : g_table_index_info)
	{
		data = iter.first + ":";
		for (auto column_name : iter.second)
		{
			data += column_name + ", ";
		}
		writeLog("new_ir_tree.log", data.c_str());
	}

	writeLog("new_ir_tree.log", "all constraint:");
	for (auto iter : g_table_constraint_info)
	{
		data = iter.first + ":";
		for (auto column_name : iter.second)
		{
			data += column_name + ", ";
		}
		writeLog("new_ir_tree.log", data.c_str());
	}
}

bool check_dump(const string &filename)
{
	ifstream file("filename");
	if (!file.is_open())
		return false;
	if (file.peek() == std::ifstream::traits_type::eof())
		return false;
	return true;
}

void tables_init()
{
	g_table_names.clear();
	g_view_names.clear();
	g_table_column_info.clear();
	g_table_constraint_info.clear();
	g_table_index_info.clear();
	vector<string>().swap(g_table_names);
	vector<string>().swap(g_view_names);
	map<string, vector<string>>().swap(g_table_column_info);
	map<string, vector<string>>().swap(g_table_constraint_info);
	map<string, vector<string>>().swap(g_table_index_info);
	malloc_trim(0);
}

void parse_sql_file(const string &filename)
{
	ifstream file(filename);
	string line;
	string current_table;

	regex create_table_regex("^CREATE TABLE `(\\w+)` \\(");
	regex column_regex("^  `(\\w+)` .+$");
	regex constraint_regex("^  CONSTRAINT `(\\w+)` .+$");
	regex index_regex("^  (KEY|INDEX|UNIQUE KEY) `(\\w+)` .+$");
	regex view_regex("^.+ CREATE VIEW `(\\w+)` .+$");
	regex column_view_regex(".+ `(\\w+)`.*$");

	smatch matches;
	bool is_view = false;

	if (!file.is_open())
	{
		cerr << "Failed to open file: " << filename << endl;
		return;
	}

	while (getline(file, line))
	{
		if (regex_search(line, matches, create_table_regex))
		{
			current_table = matches[1];
			g_table_names.push_back(current_table);
			is_view = false;
		}
		else if (regex_search(line, matches, view_regex))
		{
			current_table = matches[1];
			g_view_names.push_back(current_table);
			is_view = true;
		}
		else if (is_view)
		{
			if (regex_search(line, matches, column_view_regex))
			{
				g_table_column_info[current_table].push_back(matches[1]);
			}
		}
		else
		{
			if (regex_search(line, matches, column_regex))
			{
				g_table_column_info[current_table].push_back(matches[1]);
			}
			else if (regex_search(line, matches, constraint_regex))
			{
				g_table_constraint_info[current_table].push_back(matches[1]);
			}
			else if (regex_search(line, matches, index_regex))
			{
				g_table_index_info[current_table].push_back(matches[2]);
			}
		}
	}
	file.close();
}

void update_database()
{
	system("./update_database.sh");
	if (check_dump("./database.sql"))
	{
		tables_init();
		parse_sql_file("./database.sql");
	}
}

void drops()
{
	for (auto &c : drop_tables)
	{
		g_table_names.erase(remove(g_table_names.begin(), g_table_names.end(), c), g_table_names.end());
		g_table_column_info.erase(c);
		g_table_constraint_info.erase(c);
		g_table_index_info.erase(c);
	}

	for (auto &c : drop_views)
	{
		g_view_names.erase(remove(g_view_names.begin(), g_view_names.end(), c), g_view_names.end());
		g_table_column_info.erase(c);
		g_table_constraint_info.erase(c);
		g_table_index_info.erase(c);
	}
}

void creates()
{
	system("> tables.sql");
	for (auto &c : create_tables)
	{
		string cmd = "/usr/local/mysql/bin/mysqldump -u root test1 " + c + " --no-data --compact >> tables.sql 2> /dev/null";
		system(cmd.c_str());
	}
	for (auto &c : create_views)
	{
		string cmd = "/usr/local/mysql/bin/mysqldump -u root test1 " + c + " --no-data --compact >> tables.sql 2> /dev/null";
		system(cmd.c_str());
	}
	if (check_dump("./tables.sql"))
	{
		parse_sql_file("./tables.sql");
	}
}

void update_tables()
{
	drops();
	creates();

	create_tables.clear();
	drop_tables.clear();
	create_views.clear();
	drop_views.clear();
	set<string>().swap(create_tables);
	set<string>().swap(drop_tables);
	set<string>().swap(create_views);
	set<string>().swap(drop_views);
	malloc_trim(0);
}



void init_table()
{

	g_table_names.push_back("v0");

	g_table_column_info["v0"].push_back("v1");
	g_table_column_info["v0"].push_back("v2");
	g_table_column_info["v0"].push_back("v3");

	g_table_names.push_back("v4");

	g_table_column_info["v4"].push_back("v5");
	g_table_column_info["v4"].push_back("v6");
	g_table_column_info["v4"].push_back("v7");
}


void free_all()
{

	g_stmt_graph.column_to_table.clear();
	g_stmt_graph.column_to_column.clear();
	g_stmt_graph.not_used_table.clear();

	for (auto column : g_all_column)
	{
		delete column;
	}
	g_all_column.clear();

	for (auto table : g_all_table)
	{
		delete table;
	}
	g_all_table.clear();

	for (auto table : g_all_certain_table)
	{
		delete table;
	}
	g_all_certain_table.clear();
	g_used_table.clear();


	vector<s_column *>().swap(g_all_column);
	vector<s_table *>().swap(g_all_table);
	vector<certain_table *>().swap(g_all_certain_table);
	vector<IR *>().swap(g_stmt_graph.not_used_table);
	map<s_column *, certain_table *>().swap(g_stmt_graph.column_to_table);
	map<s_column *, certain_table *>().swap(g_stmt_graph.column_to_column);
	set<IR *>().swap(g_used_table);
	malloc_trim(0);
}
void my_validate_init()
{


	t_table_names_add.clear();
	t_table_names_del.clear();
	t_view_names_add.clear();
	t_view_names_del.clear();
	t_trigger_names_add.clear();
	t_trigger_names_del.clear();

	t_table_column_info_add.clear();
	t_table_column_info_del.clear();
	t_table_constraint_info_add.clear();
	t_table_constraint_info_del.clear();
	t_table_index_info_add.clear();
	t_table_index_info_del.clear();

	vector<string>().swap(t_table_names_add);
	vector<string>().swap(t_table_names_del);
	vector<string>().swap(t_view_names_add);
	vector<string>().swap(t_view_names_del);
	vector<string>().swap(t_trigger_names_add);
	vector<string>().swap(t_trigger_names_del);

	map<string, vector<string>>().swap(t_table_column_info_add);
	map<string, vector<string>>().swap(t_table_column_info_del);
	map<string, vector<string>>().swap(t_table_constraint_info_add);
	map<string, vector<string>>().swap(t_table_constraint_info_del);
	map<string, vector<string>>().swap(t_table_index_info_add);
	map<string, vector<string>>().swap(t_table_index_info_del);

	malloc_trim(0);
}

void validate_init()
{
	g_table_names.clear();
	g_view_names.clear();
	g_trigger_names.clear();
	g_table_column_info.clear();
	g_table_constraint_info.clear();
	g_table_index_info.clear();

	vector<string>().swap(g_table_names);
	vector<string>().swap(g_view_names);
	vector<string>().swap(g_trigger_names);
	map<string, vector<string>>().swap(g_table_column_info);
	map<string, vector<string>>().swap(g_table_constraint_info);
	map<string, vector<string>>().swap(g_table_index_info);
	malloc_trim(0);
}

void traverse_stmt(IR *root)
{

	vector<IR *> v_kStmt;
	IR *node_kStmtlist = root->left_;

	v_kStmt.push_back(node_kStmtlist->left_);
	while (node_kStmtlist->right_)
	{
		node_kStmtlist = node_kStmtlist->right_;
		v_kStmt.push_back(node_kStmtlist->left_);
	}

	for (auto node_kStmt : v_kStmt)
	{
		
		if (node_kStmt->case_idx_ == CASE0)
		{
			IR *node_kCreateStmt = node_kStmt->left_;
			handle_kCreateStmt(node_kCreateStmt);
		}
		
		if (node_kStmt->case_idx_ == CASE1)
		{
			IR *node_kDropStmt = node_kStmt->left_;
			handle_kDropStmt(node_kDropStmt);
		}
		
		if (node_kStmt->case_idx_ == CASE2)
		{
			IR *node_kSelectStmt = node_kStmt->left_;
			handle_kSelectStmt(node_kSelectStmt);
			
			fill_graph();
		}
		
		if (node_kStmt->case_idx_ == CASE3)
		{
			IR *node_kUpdateStmt = node_kStmt->left_;
			handle_kUpdateStmt(node_kUpdateStmt);
		}
		
		if (node_kStmt->case_idx_ == CASE4)
		{
			IR *node_kInsertStmt = node_kStmt->left_;
			handle_kInsertStmt(node_kInsertStmt);
		}
		
		if (node_kStmt->case_idx_ == CASE5)
		{
			IR *node_kAlterStmt = node_kStmt->left_;
			handle_kAlterStmt(node_kAlterStmt);
		}
		
		
		free_all();
	}
	
}

void handle_kDropStmt(IR *node_kDropStmt)
{
	if (node_kDropStmt->case_idx_ == CASE0)
	{
		IR *node_kDropIndexStmt = node_kDropStmt->left_;
		handle_kDropIndexStmt(node_kDropIndexStmt);
	}
	if (node_kDropStmt->case_idx_ == CASE1)
	{
		IR *node_kDropTableStmt = node_kDropStmt->left_;
		handle_kDropTableStmt(node_kDropTableStmt);
	}
	if (node_kDropStmt->case_idx_ == CASE2)
	{
		IR *node_kDropTriggerStmt = node_kDropStmt->left_;
		handle_kDropTriggerStmt(node_kDropTriggerStmt);
	}
	if (node_kDropStmt->case_idx_ == CASE3)
	{
		IR *node_kDropViewStmt = node_kDropStmt->left_;
		handle_kDropViewStmt(node_kDropViewStmt);
	}
}



void handle_kDropIndexStmt(IR *node_kDropIndexStmt)
{
	IR *index_name = node_kDropIndexStmt->left_->left_->left_;
	IR *table_name = node_kDropIndexStmt->left_->right_->left_;

	vector<string> table_have_index;
	for (auto iter : g_table_index_info)
	{
		table_have_index.push_back(iter.first);
	}
	string cur_table_name = vector_rand_ele_str(table_have_index);
	string cur_index_name = vector_rand_ele_str(g_table_index_info[cur_table_name]);
	table_name->str_val_ = cur_table_name;
	index_name->str_val_ = cur_index_name;

	t_table_index_info_del[cur_table_name].push_back(cur_index_name);

}
void handle_kDropTableStmt(IR *node_kDropTableStmt)
{

	IR *node_kTableNameList = node_kDropTableStmt->left_->right_;
	vector<IR *> v_table_name;
	v_table_name.push_back(node_kTableNameList->left_->left_);
	while (node_kTableNameList->right_)
	{
		node_kTableNameList = node_kTableNameList->right_;
		v_table_name.push_back(node_kTableNameList->left_->left_);
	}

	for (auto table_name : v_table_name)
	{
		string cur_table_name = vector_rand_ele_str(g_table_names);
		table_name->str_val_ = cur_table_name;
		t_table_names_del.push_back(cur_table_name);
		
		
		
	}
}

void handle_kDropTriggerStmt(IR *node_kDropTriggerStmt)
{

	IR *trigger_name = node_kDropTriggerStmt->right_->left_;
	string cur_trigger_name = vector_rand_ele_str(g_trigger_names);
	trigger_name->str_val_ = cur_trigger_name;
	t_trigger_names_del.push_back(cur_trigger_name);
	
}
void handle_kDropViewStmt(IR *node_kDropViewStmt)
{
	IR *view_name = node_kDropViewStmt->left_->right_->left_;

	if (g_view_names.size() == 0)
	{
		g_return_val = false;
		return;
	}
	string cur_view_name = vector_rand_ele_str(g_view_names);
	view_name->str_val_ = cur_view_name;
	t_view_names_del.push_back(cur_view_name);

}


void handle_kUpdateStmt(IR *node_kUpdateStmt)
{

	select_field_info cur_info;
	IR *node_kOptWithClause = node_kUpdateStmt->left_->left_->left_->left_->left_->left_->left_;
	cur_info.with_table = hanle_kOptWithClause(node_kOptWithClause);

	IR *node_kTableRef = node_kUpdateStmt->left_->left_->left_->left_->right_;
	handle_kTableRef(node_kTableRef, cur_info.from_table);

	IR *node_kAssignmentList = node_kUpdateStmt->left_->left_->left_->right_;
	handle_kAssignmentList(node_kAssignmentList, cur_info.selectTarget_column);

	IR *node_kOptWhereClause = node_kUpdateStmt->left_->left_->right_;
	handle_kOptWhereClause(node_kOptWhereClause, cur_info.where_column);

	IR *node_kOptOrderClause = node_kUpdateStmt->left_->right_;
	handle_kOptOrderClause(node_kOptOrderClause, cur_info.order_by_column);

	IR *node_kOptLimitRowCount = node_kUpdateStmt->right_;
	handle_kOptLimitRowCount(node_kOptLimitRowCount);
	validate_build_graph(&cur_info);

	fill_graph();
}

void handle_kAssignmentList(IR *cur_kAssignmentList, vector<s_column *> &set_column)
{

	vector<IR *> v_kAssignment;
	v_kAssignment.push_back(cur_kAssignmentList->left_);
	while (cur_kAssignmentList->right_)
	{
		cur_kAssignmentList = cur_kAssignmentList->right_;
		v_kAssignment.push_back(cur_kAssignmentList->left_);
	}
	for (auto cur_kAssignment : v_kAssignment)
	{
		IR *node_kColumnName = cur_kAssignment->left_;
		s_column *ret_column = handle_kColumnName(node_kColumnName);
		set_column.push_back(ret_column);

		IR *node_kExpr = cur_kAssignment->right_;
		dfs_traverse_kExpr(node_kExpr, set_column);
	}
}
void handle_kOptLimitRowCount(IR *node_kOptLimitRowCount)
{
	if (node_kOptLimitRowCount->left_)
	{
		vector<s_column *> temp_column;
		dfs_traverse_kExpr(node_kOptLimitRowCount->left_, temp_column);
	}
}

void handle_kInsertStmt(IR *node_kInsertStmt)
{

	select_field_info cur_info;
	IR *table_name;
	IR *node_kOptColumnNameListP;
	IR *node_kOptAsRowAlias;

	
	if (node_kInsertStmt->case_idx_ == CASE0)
	{
		table_name = node_kInsertStmt->left_->left_->left_->left_->left_->right_->left_;

		node_kOptColumnNameListP = node_kInsertStmt->left_->left_->left_->left_->right_;
		handle_kOptColumnNameListP(node_kOptColumnNameListP, cur_info.selectTarget_column);

		node_kOptAsRowAlias = node_kInsertStmt->left_->right_;

		IR *node_kSuperValueList = node_kInsertStmt->left_->left_->right_;
		
		dfs_traverse_kExpr(node_kSuperValueList, cur_info.selectTarget_column);
	}
	
	if (node_kInsertStmt->case_idx_ == CASE1)
	{
		table_name = node_kInsertStmt->left_->left_->left_->right_->left_;

		node_kOptAsRowAlias = node_kInsertStmt->left_->left_->right_;

		IR *node_kAssignmentList = node_kInsertStmt->left_->right_;
		handle_kAssignmentList(node_kAssignmentList, cur_info.selectTarget_column);
	}
	
	if (node_kInsertStmt->case_idx_ == CASE2)
	{
		table_name = node_kInsertStmt->left_->left_->left_->left_->right_->left_;

		node_kOptColumnNameListP = node_kInsertStmt->left_->left_->left_->right_;
		handle_kOptColumnNameListP(node_kOptColumnNameListP, cur_info.selectTarget_column);

		node_kOptAsRowAlias = node_kInsertStmt->left_->left_->right_;

		IR *node_kSelectNoParens = node_kInsertStmt->left_->right_;
		handle_kSelectNoParens(node_kSelectNoParens);

	}
	
	if (node_kInsertStmt->case_idx_ == CASE3)
	{
		table_name = node_kInsertStmt->left_->left_->left_->left_->right_->left_;

		node_kOptColumnNameListP = node_kInsertStmt->left_->left_->left_->right_;
		handle_kOptColumnNameListP(node_kOptColumnNameListP, cur_info.selectTarget_column);

		node_kOptAsRowAlias = node_kInsertStmt->left_->left_->right_;

		IR *table_name_2 = node_kInsertStmt->left_->right_->left_;
		table_name_2->str_val_ = vector_rand_ele_str(g_table_names);
	}

	s_table *cur_table = new s_table;
	cur_table->v_TableName.push_back(table_name);
	g_all_table.push_back(cur_table);
	cur_info.from_table.push_back(cur_table);

	if (node_kOptAsRowAlias->left_)
	{
		IR *alias_name = node_kOptAsRowAlias->left_->left_;
		string cur_alias_name = gen_id_name();
		alias_name->str_val_ = cur_alias_name;
		cur_table->alias = cur_alias_name;

		IR *node_kOptColAlias = node_kOptAsRowAlias->right_;
		if (node_kOptColAlias->left_)
		{
			IR *node_kColAliasList = node_kOptColAlias->left_;
			vector<IR *> v_col_alias;
			v_col_alias.push_back(node_kColAliasList->left_->left_);
			while (node_kColAliasList->right_)
			{
				node_kColAliasList = node_kColAliasList->right_;
				v_col_alias.push_back(node_kColAliasList->left_->left_);
			}
			for (auto col_alias : v_col_alias)
			{
				col_alias->str_val_ = gen_id_name();
			}
		}
	}

	IR *node_kOptOnDuplicate = node_kInsertStmt->right_;
	if (node_kOptOnDuplicate->left_)
	{
		handle_kAssignmentList(node_kOptOnDuplicate->left_, cur_info.selectTarget_column);
	}

	validate_build_graph(&cur_info);
	fill_graph();
}

void handle_kOptColumnNameListP(IR *node_kOptColumnNameListP, vector<s_column *> &ret_column)
{
	vector<IR *> column_name;
	dfs_traverse_kOptColumnNameListP(node_kOptColumnNameListP, column_name);
	for (auto cur_column_name : column_name)
	{
		s_column *cur_column = new s_column;
		cur_column->ColumnName = cur_column_name;
		g_all_column.push_back(cur_column);
		ret_column.push_back(cur_column);
	}
}
void handle_kAlterStmt(IR *node_kAlterStmt)
{
	IR *table_name = node_kAlterStmt->left_->left_;
	string cur_table_name = vector_rand_ele_str(g_table_names);
	table_name->str_val_ = cur_table_name;

	IR *node_kAlterAction = node_kAlterStmt->right_;
	
	if (node_kAlterAction->case_idx_ == CASE0)
	{
		IR *table_name_2 = node_kAlterAction->left_->left_;
		string new_table_name = gen_id_name();
		table_name_2->str_val_ = new_table_name;
		t_table_names_del.push_back(cur_table_name);
		t_table_names_add.push_back(new_table_name);

		
		

		
		
		
		
	}
	
	if (node_kAlterAction->case_idx_ == CASE1)
	{
		IR *column_name_1 = node_kAlterAction->left_->left_;
		IR *column_name_2 = node_kAlterAction->right_->left_;

		string old_column_name = vector_rand_ele_str(g_table_column_info[cur_table_name]);
		string new_column_name = gen_id_name();

		column_name_1->str_val_ = old_column_name;
		column_name_2->str_val_ = new_column_name;
		t_table_names_del.push_back(cur_table_name);
		t_table_names_add.push_back(cur_table_name);
		t_table_column_info_add[cur_table_name].push_back(new_column_name);
		t_table_column_info_del[cur_table_name].push_back(old_column_name);

		
		
		
		
	}
	
	if (node_kAlterAction->case_idx_ == CASE2)
	{
		IR *column_name = node_kAlterAction->right_->left_->left_;
		string add_column_name = gen_id_name();
		column_name->str_val_ = add_column_name;
		t_table_names_del.push_back(cur_table_name);
		t_table_names_add.push_back(cur_table_name);
		
		t_table_column_info_add[cur_table_name].push_back(add_column_name);
		
		
	}
	
	if (node_kAlterAction->case_idx_ == CASE3)
	{
		IR *column_name = node_kAlterAction->right_->left_;
		string drop_column_name = vector_rand_ele_str(g_table_column_info[cur_table_name]);
		column_name->str_val_ = drop_column_name;
		t_table_names_del.push_back(cur_table_name);
		t_table_names_add.push_back(cur_table_name);
		t_table_column_info_del[cur_table_name].push_back(drop_column_name);
		
		
		
	}
	
	

	
	if (node_kAlterAction->case_idx_ == CASE5)
	{
		IR *node_kColumnDefList = node_kAlterAction->right_;
		handle_kColumnDefList(cur_table_name, node_kColumnDefList);
		
		
	}
}

void handle_kColumnDefList(string cur_table_name, IR *node_kColumnDefList)
{
	vector<IR *> v_kColumnDef;
	while (node_kColumnDefList->left_->type_ == kColumnDefList)
	{
		v_kColumnDef.push_back(node_kColumnDefList->right_);
		node_kColumnDefList = node_kColumnDefList->left_;
	}
	v_kColumnDef.push_back(node_kColumnDefList->left_);
	for (auto node_kColumnDef : v_kColumnDef)
	{
		IR *column_name = node_kColumnDef->left_->left_;
		string cur_column_name = gen_id_name();
		column_name->str_val_ = cur_column_name;
		t_table_column_info_add[cur_table_name].push_back(cur_column_name);
		
	}
}

unsigned int calc_node(IR *root)
{
	unsigned int res = 0;
	if (root->left_)
		res += calc_node(root->left_);
	if (root->right_)
		res += calc_node(root->right_);

	return res + 1;
}

bool my_validate(IR *&root)
{
	string sql = root->to_string() + " ;";
	
	

	auto ast = parser(sql);
	if (ast == NULL)
	{
		return false;
	}
	deep_delete(root);
	root = NULL;
	vector<IR *> ir_vector;
	ast->translate(ir_vector);
	ast->deep_delete();
	root = ir_vector[ir_vector.size() - 1];

	int validate_count = 0;
	bool retVal = false;


	while (validate_count < 5)
	{
		if (my_fix(root))
		{
			retVal = true;
			break;
		}
		validate_count++;
	}

	return retVal;
}

bool my_fix(IR *&root)
{
	g_return_val = true;
	my_validate_init();
	traverse_stmt(root);
	return g_return_val;
}

void print_table()
{

	cout << "all table:" << endl;
	for (auto iter : g_table_column_info)
	{

		cout << iter.first << ":";
		for (auto column_name : iter.second)
		{
			cout << column_name << ", ";
		}
		cout << endl;
	}

	cout << "all index:" << endl;
	for (auto iter : g_table_index_info)
	{

		cout << iter.first << ":";
		for (auto column_name : iter.second)
		{
			cout << column_name << ", ";
		}
		cout << endl;
	}
}

void print_graph()
{

	cout << "column --> table:" << endl;

	for (auto &iter : g_stmt_graph.column_to_table)
	{

		if (iter.first->TableName)
		{

			cout << iter.first->TableName->str_val_ << ".";
		}
		cout << iter.first->ColumnName->str_val_;
		cout << "----->";
		if (iter.second->alias.empty())
		{
			cout << iter.second->rely_table->str_val_;
		}
		else
		{
			cout << iter.second->alias;
		}
		cout << endl;
	}

	cout << "column --> column:" << endl;

	for (auto &iter : g_stmt_graph.column_to_column)
	{

		if (iter.first->TableName)
		{

			cout << iter.first->TableName->str_val_ << ".";
		}
		cout << iter.first->ColumnName->str_val_;

		cout << "----->";

		if (iter.second->rely_column->TableName)
		{

			cout << iter.second->rely_column->TableName->str_val_ << ".";
		}
		if (iter.second->alias.empty())
		{
			cout << iter.second->rely_column->ColumnName->str_val_;
		}
		else
		{
			cout << iter.second->alias;
		}

		cout << endl;
	}

	cout << "not_used_table:" << endl;

	for (auto IR_table : g_stmt_graph.not_used_table)
	{

		cout << IR_table->str_val_ << endl;
	}
}

void handle_kCreateStmt(IR *node_kCreateStmt)
{

	
	if (node_kCreateStmt->case_idx_ == CASE0)
	{

		IR *node_CreateTableStmt = node_kCreateStmt->left_;
		handle_CreateTableStmt(node_CreateTableStmt);
	}

	if (node_kCreateStmt->case_idx_ == CASE1)
	{

		IR *node_CreateIndexStmt = node_kCreateStmt->left_;
		handle_CreateIndexStmt(node_CreateIndexStmt);
	}

	if (node_kCreateStmt->case_idx_ == CASE2)
	{

		IR *node_CreateTriggerStmt = node_kCreateStmt->left_;
		handle_CreateTriggerStmt(node_CreateTriggerStmt);
	}

	if (node_kCreateStmt->case_idx_ == CASE3)
	{

		IR *node_CreateViewStmt = node_kCreateStmt->left_;
		handle_CreateViewStmt(node_CreateViewStmt);
	}
}

void handle_CreateTableStmt(IR *node_CreateTableStmt)
{
	
	if (node_CreateTableStmt->case_idx_ == CASE0)
	{

		IR *table_name = node_CreateTableStmt->left_->left_->right_->left_;
		string cur_table_name = gen_id_name();
		table_name->str_val_ = cur_table_name;
		
		t_table_names_add.push_back(cur_table_name);

		IR *node_kCreateDefinitionList = node_CreateTableStmt->left_->right_;
		handle_kCreateDefinitionList(node_kCreateDefinitionList, cur_table_name);
	}

	
	if (node_CreateTableStmt->case_idx_ == CASE1)
	{

		IR *table_name = node_CreateTableStmt->left_->left_->left_->left_->left_->right_->left_;
		string cur_table_name = gen_id_name();
		
		t_table_names_add.push_back(cur_table_name);
		
		table_name->str_val_ = cur_table_name;

		IR *node_kSelectStmt = node_CreateTableStmt->right_;
		s_table *select_result = handle_kSelectStmt(node_kSelectStmt);
		fill_graph();

		vector<string> column_name;
		for (auto cur_column : select_result->v_ColumnName)
		{
			if ((cur_column->ColumnName == NULL) && (cur_column->alias.size() != 0))
			{
				column_name.push_back(cur_column->alias);
			}
			if (cur_column->ColumnName != NULL)
			{
				column_name.push_back(cur_column->ColumnName->str_val_);
			}
		}

		for (auto cur_table : select_result->v_TableName)
		{
			string cur_table_name = cur_table->str_val_;
			for (auto cur_column_name : t_table_column_info_add[cur_table_name])
			{
				column_name.push_back(cur_column_name);
			}
		}


		IR *node_kOptCreateDefinitionListP = node_CreateTableStmt->left_->left_->left_->left_->right_;
		if (node_kOptCreateDefinitionListP->left_)
		{
			IR *node_kCreateDefinitionList = node_kOptCreateDefinitionListP->left_;
			handle_kCreateDefinitionList(node_kCreateDefinitionList, cur_table_name);
		}

		else
		{

			set<string> column_name_set(column_name.begin(), column_name.end());
			if (column_name_set.size() != column_name.size())
			{
				g_return_val = false;
				return;
			}
			t_table_column_info_add[cur_table_name].insert(t_table_column_info_add[cur_table_name].end(), column_name.begin(), column_name.end());
			
		}
	}

	
	if (node_CreateTableStmt->case_idx_ == CASE2)
	{

		IR *like_table_name = node_CreateTableStmt->right_->left_->left_;
		string cur_like_table_name = vector_rand_ele_str(g_table_names);
		like_table_name->str_val_ = cur_like_table_name;

		IR *table_name = node_CreateTableStmt->left_->right_->left_;
		string cur_table_name = gen_id_name();
		t_table_names_add.push_back(cur_table_name);
		
		
		table_name->str_val_ = cur_table_name;
		t_table_column_info_add[cur_table_name].insert(t_table_column_info_add[cur_table_name].end(), g_table_column_info[cur_like_table_name].begin(), g_table_column_info[cur_like_table_name].end());
		
	}
}
void handle_kCreateDefinitionList(IR *node_kCreateDefinitionList, string cur_table_name)
{
	vector<IR *> v_kCreateDefinition_1;

	vector<IR *> v_kCreateDefinition_2;

	IR *cur_kCreateDefinition = node_kCreateDefinitionList->left_;
	if (cur_kCreateDefinition->case_idx_ == CASE0)
	{
		v_kCreateDefinition_1.push_back(cur_kCreateDefinition);
	}
	else
	{
		v_kCreateDefinition_2.push_back(cur_kCreateDefinition);
	}
	while (node_kCreateDefinitionList->right_)
	{
		node_kCreateDefinitionList = node_kCreateDefinitionList->right_;
		cur_kCreateDefinition = node_kCreateDefinitionList->left_;
		if (cur_kCreateDefinition->case_idx_ == CASE0)
		{
			v_kCreateDefinition_1.push_back(cur_kCreateDefinition);
		}
		else
		{
			v_kCreateDefinition_2.push_back(cur_kCreateDefinition);
		}
	}


	for (auto node_kCreateDefinition : v_kCreateDefinition_1)
	{
		handle_kCreateDefinition(node_kCreateDefinition, cur_table_name);
	}

	for (auto node_kCreateDefinition : v_kCreateDefinition_2)
	{
		handle_kCreateDefinition(node_kCreateDefinition, cur_table_name);
	}
}

void handle_kCreateDefinition(IR *node_kCreateDefinition, string cur_table_name)
{


	if (node_kCreateDefinition->case_idx_ == CASE0)
	{
		IR *column_name = node_kCreateDefinition->left_->left_;
		string cur_column_name = gen_id_name();
		column_name->str_val_ = cur_column_name;
		t_table_column_info_add[cur_table_name].push_back(cur_column_name);
		
		IR *node_kColumnDefinition = node_kCreateDefinition->right_;
		handle_kColumnDefinition(node_kColumnDefinition, cur_column_name, cur_table_name);
	}
	if (node_kCreateDefinition->case_idx_ == CASE1 || node_kCreateDefinition->case_idx_ == CASE2)
	{
		IR *node_kOptIndexName;
		if (node_kCreateDefinition->case_idx_ == CASE1)
		{
			node_kOptIndexName = node_kCreateDefinition->left_->left_->left_->right_;
		}
		else
		{
			node_kOptIndexName = node_kCreateDefinition->left_->left_->right_;
		}
		handle_kOptIndexName(node_kOptIndexName, cur_table_name);

		IR *node_kKeyPartList = node_kCreateDefinition->left_->right_;
		handle_kKeyPartList(node_kKeyPartList, cur_table_name);
	}
	
	if (node_kCreateDefinition->case_idx_ == CASE3)
	{
		IR *node_kOptConstraintSymbol = node_kCreateDefinition->left_->left_->left_;
		handle_kOptConstraintSymbol(node_kOptConstraintSymbol, cur_table_name);

		IR *node_kKeyPartList = node_kCreateDefinition->left_->right_;
		handle_kKeyPartList(node_kKeyPartList, cur_table_name);
	}
	
	if (node_kCreateDefinition->case_idx_ == CASE4)
	{
		IR *node_kOptConstraintSymbol = node_kCreateDefinition->left_->left_->left_->left_->left_;
		handle_kOptConstraintSymbol(node_kOptConstraintSymbol, cur_table_name);

		IR *node_kOptIndexName = node_kCreateDefinition->left_->left_->left_->right_;
		handle_kOptIndexName(node_kOptIndexName, cur_table_name);

		IR *node_kKeyPartList = node_kCreateDefinition->left_->right_;
		handle_kKeyPartList(node_kKeyPartList, cur_table_name);
	}
	
	if (node_kCreateDefinition->case_idx_ == CASE5)
	{

		if (g_table_names.size() == 1)
		{
			g_return_val = false;
			return;
		}

		IR *node_kOptConstraintSymbol = node_kCreateDefinition->left_->left_->left_;
		handle_kOptConstraintSymbol(node_kOptConstraintSymbol, cur_table_name);

		IR *node_kOptIndexName = node_kCreateDefinition->left_->left_->right_;
		handle_kOptIndexName(node_kOptIndexName, cur_table_name);

		vector<s_column *> temp_columns;
		IR *node_kColumnNameList = node_kCreateDefinition->left_->right_;
		handle_kColumnNameList(node_kColumnNameList, temp_columns);
		for (auto cur_column : temp_columns)
		{
			cur_column->ColumnName->str_val_ = vector_rand_ele_str(t_table_column_info_add[cur_table_name]);
		}

		IR *node_kReferenceDefinition = node_kCreateDefinition->right_;
		handle_kReferenceDefinition(node_kReferenceDefinition, cur_table_name);
	}
	
	if (node_kCreateDefinition->case_idx_ == CASE6)
	{
		IR *kCheckConstraintDefinition = node_kCreateDefinition->left_;
		handle_kCheckConstraintDefinition(kCheckConstraintDefinition, string("table_level"), cur_table_name);
	}
}
void handle_kOptIndexName(IR *node_kOptIndexName, string cur_table_name)
{
	if (node_kOptIndexName->left_)
	{
		IR *index_name = node_kOptIndexName->left_->left_;
		string cur_index_name = gen_id_name();
		index_name->str_val_ = cur_index_name;
		t_table_index_info_add[cur_table_name].push_back(cur_index_name);
		
	}
}

void handle_kColumnDefinition(IR *node_kColumnDefinition, string cur_column_name, string cur_table_name)
{
	IR *node_kOptCheckConstraintDefinition = node_kColumnDefinition->right_;
	if (node_kOptCheckConstraintDefinition->left_)
	{
		handle_kCheckConstraintDefinition(node_kOptCheckConstraintDefinition->left_, cur_column_name, cur_table_name);
	}

	IR *node_KOptReferenceDefinition = node_kColumnDefinition->left_->right_;
}


void handle_kCheckConstraintDefinition(IR *node_kCheckConstraintDefinition, string cur_column_name, string cur_table_name)
{
	IR *node_kOptConstraintSymbol = node_kCheckConstraintDefinition->left_->left_;
	handle_kOptConstraintSymbol(node_kOptConstraintSymbol, cur_table_name);

	vector<s_column *> check_column;
	IR *node_kExpr = node_kCheckConstraintDefinition->left_->right_;
	dfs_traverse_kExpr(node_kExpr, check_column);

	if (cur_column_name == "table_level")
	{
		for (auto cur_check_column : check_column)
		{
			cur_check_column->ColumnName->str_val_ = vector_rand_ele_str(g_table_column_info[cur_table_name]);
		}
	}
	else
	{
		for (auto cur_check_column : check_column)
		{
			cur_check_column->ColumnName->str_val_ = cur_column_name;
		}
	}
}

void handle_kOptConstraintSymbol(IR *node_kOptConstraintSymbol, string cur_table_name)
{
	if (node_kOptConstraintSymbol->left_)
	{
		IR *constraint_symbol = node_kOptConstraintSymbol->left_->left_;
		string cur_constraint_symbol = gen_id_name();
		constraint_symbol->str_val_ = cur_constraint_symbol;
		t_table_constraint_info_add[cur_table_name].push_back(cur_constraint_symbol);
		
	}
}

void handle_kReferenceDefinition(IR *node_kReferenceDefinition, string cur_table_name)
{
	IR *table_name = node_kReferenceDefinition->left_->left_->left_->left_->left_;
	vector<string> tables_except_cur = g_table_names;
	for (auto iter = tables_except_cur.begin(); iter != tables_except_cur.end();)
	{
		if (*iter == cur_table_name)
			iter = tables_except_cur.erase(iter);
		else
		{
			iter++;
		}
	}
	string foreign_key_table = vector_rand_ele_str(tables_except_cur);
	table_name->str_val_ = foreign_key_table;

	IR *node_kKeyPartList = node_kReferenceDefinition->left_->left_->left_->right_;
	handle_kKeyPartList(node_kKeyPartList, foreign_key_table);
}

void handle_CreateIndexStmt(IR *node_CreateIndexStmt)
{

	IR *index_name = node_CreateIndexStmt->left_->left_->left_->left_->left_->right_->left_;
	string cur_index_name = gen_id_name();
	index_name->str_val_ = cur_index_name;

	IR *table_name = node_CreateIndexStmt->left_->left_->left_->right_->left_;
	string cur_table_name = vector_rand_ele_str(g_table_names);
	table_name->str_val_ = cur_table_name;

	t_table_index_info_add[cur_table_name].push_back(cur_index_name);
	
	
	

	IR *node_kKeyPartList = node_CreateIndexStmt->left_->left_->right_;
	handle_kKeyPartList(node_kKeyPartList, cur_table_name);
}

void handle_kKeyPartList(IR *node_kKeyPartList, string cur_table_name)
{
	vector<s_column *> index_column;
	dfs_traverse_kExpr(node_kKeyPartList, index_column);

	for (auto cur_column : index_column)
	{
		cur_column->ColumnName->str_val_ = vector_rand_ele_str(g_table_column_info[cur_table_name]);
	}
}

void handle_CreateTriggerStmt(IR *node_CreateTriggerStmt)
{

	IR *trigger_name = node_CreateTriggerStmt->left_->left_->left_->left_->left_;
	string cur_trigger_name = gen_id_name();
	trigger_name->str_val_ = cur_trigger_name;
	t_trigger_names_add.push_back(cur_trigger_name);
	
	

	IR *table_name = node_CreateTriggerStmt->left_->right_->left_;
	table_name->str_val_ = vector_rand_ele_str(g_table_names);

	IR *node_kTriggerBody = node_CreateTriggerStmt->right_;
	handle_kTriggerBody(node_kTriggerBody);
}

void handle_kTriggerBody(IR *node_kTriggerBody)
{
	
	if (node_kTriggerBody->case_idx_ == CASE0)
	{
		handle_kDropStmt(node_kTriggerBody->left_);
	}
	if (node_kTriggerBody->case_idx_ == CASE1)
	{
		handle_kUpdateStmt(node_kTriggerBody->left_);
	}
	if (node_kTriggerBody->case_idx_ == CASE2)
	{
		handle_kInsertStmt(node_kTriggerBody->left_);
	}
	if (node_kTriggerBody->case_idx_ == CASE3)
	{
		handle_kAlterStmt(node_kTriggerBody->left_);
	}
}

void handle_CreateViewStmt(IR *node_CreateViewStmt)
{

	IR *view_name = node_CreateViewStmt->left_->left_->left_->right_->left_;
	string cur_view_name = gen_id_name();
	view_name->str_val_ = cur_view_name;

	IR *node_kOptColumnNameListP = node_CreateViewStmt->left_->left_->right_;
	vector<IR *> column_name;
	dfs_traverse_kOptColumnNameListP(node_kOptColumnNameListP, column_name);

	IR *node_kSelectStmt = node_CreateViewStmt->left_->right_;
	s_table *ret_table = handle_kSelectStmt(node_kSelectStmt);
	fill_graph();

	
	if (column_name.size() != 0)
	{
		for (auto cur_column_name : column_name)
		{
			string str_column_name = gen_id_name();
			cur_column_name->str_val_ = str_column_name;
			t_table_column_info_add[cur_view_name].push_back(str_column_name);

		}
	}
	else
	{
		for (auto cur_column : ret_table->v_ColumnName)
		{
			if (cur_column->alias.size() != 0)
			{
				t_table_column_info_add[cur_view_name].push_back(cur_column->alias);
				
				
			}
			else
			{
				t_table_column_info_add[cur_view_name].push_back(cur_column->ColumnName->str_val_);
				
				
			}
		}
		for (auto cur_table : ret_table->v_TableName)
		{
			t_table_column_info_add[cur_view_name].insert(t_table_column_info_add[cur_view_name].end(), g_table_column_info[cur_table->str_val_].begin(), g_table_column_info[cur_table->str_val_].end());
			
			
		}
	}
	if (t_table_column_info_add[cur_view_name].size() != 0)
	{
		
		t_view_names_add.push_back(cur_view_name);
	}
}

void dfs_traverse_kOptColumnNameListP(IR *node, vector<IR *> &column_name)
{

	if (node->type_ == kIdentifier)
	{
		column_name.push_back(node);
	}

	if (node->left_)
	{
		dfs_traverse_kOptColumnNameListP(node->left_, column_name);
	}
	if (node->right_)
	{
		dfs_traverse_kOptColumnNameListP(node->right_, column_name);
	}
}

s_table *handle_kSelectStmt(IR *node_kSelectStmt)
{

	s_table *ret_table;
	while (node_kSelectStmt->left_->type_ != kSelectNoParens)
	{
		node_kSelectStmt = node_kSelectStmt->left_;
	}
	IR *node_kSelectNoParens = node_kSelectStmt->left_;
	ret_table = handle_kSelectNoParens(node_kSelectNoParens);
	return ret_table;
}

string get_upper_column_name(s_column *column)
{
	for (auto &iter : g_stmt_graph.column_to_column)
	{
		if (iter.first->ColumnName == column->ColumnName)
		{
			if (!iter.second->rely_column->alias.empty())
			{
				return iter.second->rely_column->alias;
			}
			else
			{
				if (iter.second->rely_column->ColumnName->str_val_ == "x")
				{
					return get_upper_column_name(iter.second->rely_column);
				}
				else
				{
					return iter.second->rely_column->ColumnName->str_val_;
				}
			}
		}
	}
	g_return_val = false;
	
	return string("v2");
}

string get_upper_rely_table_name(s_column *column)
{

	for (auto &iter : g_stmt_graph.column_to_column)
	{

		if (iter.first->ColumnName == column->ColumnName)
		{

			if ((iter.second->alias.empty()) && (iter.second->rely_column->rely_table_name.empty()))
			{

				return get_upper_rely_table_name(iter.second->rely_column);
			}
			else
			{

				if (iter.second->alias.empty())
				{

					return iter.second->rely_column->rely_table_name;
				}
				else
				{

					return iter.second->alias;
				}
			}
		}
	}
	g_return_val = false;
	
	return string("v0");
}

void fill_graph()
{

	if ((g_table_names.size() == 0) || (g_return_val == false))
	{
		return;
	}
	vector<string> table_and_view;
	table_and_view.insert(table_and_view.end(), g_table_names.begin(), g_table_names.end());
	table_and_view.insert(table_and_view.end(), g_view_names.begin(), g_view_names.end());
	for (auto &iter : g_stmt_graph.column_to_table)
	{

		s_column *cur_column = iter.first;
		IR *cur_table = iter.second->rely_table;
		string cur_table_name;
		string cur_rely_table_name;


		if (cur_table->str_val_ == "x")
		{
			cur_table->str_val_ = vector_rand_ele_str(table_and_view);
		}
		cur_table_name = cur_table->str_val_;

		if (iter.second->alias.empty())
		{
			cur_rely_table_name = cur_table_name;
		}
		else
		{
			cur_rely_table_name = iter.second->alias;
		}
		cur_column->rely_table_name = cur_rely_table_name;
		if (cur_column->TableName)
		{
			cur_column->TableName->str_val_ = cur_rely_table_name;
		}

		if (g_table_column_info.find(cur_table_name) != g_table_column_info.end())
		{
			
			if (g_table_column_info[cur_table_name].size() == 0)
			{
				g_return_val = false;
			}
			string cur_column_name = vector_rand_ele_str(g_table_column_info[cur_table_name]);
			cur_column->ColumnName->str_val_ = cur_column_name;
		}
		else
		{
			g_return_val = false;
		}
	}
	for (auto &iter : g_stmt_graph.column_to_column)
	{
		s_column *cur_column = iter.first;
		s_column *column_instantiated = iter.second->rely_column;

		string cur_column_name;
		string cur_rely_table_name;

		if (!column_instantiated->alias.empty())
		{
			cur_column_name = column_instantiated->alias;
		}
		else
		{
			cur_column_name = column_instantiated->ColumnName->str_val_;

			if (cur_column_name == "x")
			{
				cur_column_name = get_upper_column_name(column_instantiated);
			}
		}
		cur_column->ColumnName->str_val_ = cur_column_name;

		if (!iter.second->alias.empty())
		{

			cur_rely_table_name = iter.second->alias;
		}

		else if (!column_instantiated->rely_table_name.empty())
		{

			cur_rely_table_name = column_instantiated->rely_table_name;
		}

		else
		{

			cur_rely_table_name = get_upper_rely_table_name(column_instantiated);
		}
		cur_column->rely_table_name = cur_rely_table_name;

		if (cur_column->TableName)
		{

			cur_column->TableName->str_val_ = cur_rely_table_name;
		}
	}
	for (auto IR_table : g_stmt_graph.not_used_table)
	{

		IR_table->str_val_ = vector_rand_ele_str(table_and_view);
	}
}

s_table *handle_kSelectNoParens(IR *node_kSelectNoParens)
{

	
	vector<s_table *> with_table;
	IR *node_kOptWithClause = node_kSelectNoParens->left_->left_->left_;
	with_table = hanle_kOptWithClause(node_kOptWithClause);

	
	vector<s_column *> order_by_column;
	IR *node_kOptOrderClause = node_kSelectNoParens->left_->right_;
	handle_kOptOrderClause(node_kOptOrderClause, order_by_column);

	
	IR *node_kSelectClauseList = node_kSelectNoParens->left_->left_->right_;
	s_table *ret_table;

	
	if (node_kSelectClauseList->case_idx_ == CASE0)
	{
		IR *node_kSelectClause = node_kSelectClauseList->left_;

		ret_table = handle_kSelectClause(node_kSelectClause, order_by_column, with_table);
	}

	if (node_kSelectClauseList->case_idx_ == CASE1)
	{
		IR *main_kSelectClause = node_kSelectClauseList->left_->left_;
		ret_table = handle_kSelectClause(main_kSelectClause, order_by_column, with_table);
		order_by_column.clear();

		IR *cur_kSelectClauseList = node_kSelectClauseList->right_;
		IR *cur_kSelectClause;
		while (cur_kSelectClauseList->case_idx_ == CASE1)
		{
			cur_kSelectClause = cur_kSelectClauseList->left_->left_;
			handle_kSelectClause(cur_kSelectClause, order_by_column, with_table);
			cur_kSelectClauseList = cur_kSelectClauseList->right_;
		}
		cur_kSelectClause = cur_kSelectClauseList->left_;
		handle_kSelectClause(cur_kSelectClause, order_by_column, with_table);
	}
	return ret_table;
}

s_table *handle_kSelectClause(IR *node_kSelectClause, vector<s_column *> &order_by_column, vector<s_table *> &with_table)
{

	select_field_info cur_info;

	cur_info.with_table = with_table;
	cur_info.order_by_column = order_by_column;

	IR *node_kSelectTarget = node_kSelectClause->left_->left_->left_->left_->right_;
	handle_kSelectTarget(node_kSelectTarget, &cur_info);

	IR *node_kOptFromClause = node_kSelectClause->left_->left_->left_->right_;
	handle_kOptFromClause(node_kOptFromClause, cur_info.from_table);

	IR *node_kOptWhereClause = node_kSelectClause->left_->left_->right_;
	handle_kOptWhereClause(node_kOptWhereClause, cur_info.where_column);

	IR *node_kOptGroupClause = node_kSelectClause->left_->right_;
	handle_kOptGroupClause(node_kOptGroupClause, cur_info.group_by_column);

	IR *node_kOptWindowClause = node_kSelectClause->right_;
	handle_kOptWindowClause(node_kOptWindowClause, &cur_info);

	validate_build_graph(&cur_info);

	s_table *ret_table = columns_to_table(cur_info.selectTarget_column);

	return ret_table;
}

void dfs_traverse_kWindowClause(IR *node, select_field_info *p_cur_info)
{

	if (node->type_ == kWindowName)
	{
		p_cur_info->window_def.push_back(node->left_);
	}

	if (node->type_ == kWindow)
	{
		handle_kWindow(node, p_cur_info);
	}

	if (node->left_)
	{
		dfs_traverse_kWindowClause(node->left_, p_cur_info);
	}
	if (node->right_)
	{
		dfs_traverse_kWindowClause(node->right_, p_cur_info);
	}
}

void handle_kOptWindowClause(IR *node_kOptWindowClause, select_field_info *p_cur_info)
{
	dfs_traverse_kWindowClause(node_kOptWindowClause, p_cur_info);
	
	for (auto cur_window_def : p_cur_info->window_def)
	{
		cur_window_def->str_val_ = gen_id_name();
	}
	
	if (p_cur_info->window_def.size() == 0 && p_cur_info->window_name.size() != 0)
	{
		g_return_val = false;
		return;
	}
	for (auto cur_window_name : p_cur_info->window_name)
	{
		cur_window_name->str_val_ = vector_rand_ele(p_cur_info->window_def)->str_val_;
	}
}

void handle_kSelectTarget(IR *node_kSelectTarget, select_field_info *p_cur_info)
{

	vector<IR *> v_kExprList;

	IR *cur_kExprList = node_kSelectTarget->left_;

	while (strstr(ALLTYPE_enum_to_String[cur_kExprList->left_->type_], "kUnknown") != NULL)
	{

		v_kExprList.push_back(cur_kExprList->left_);
		cur_kExprList = cur_kExprList->right_;
	}

	v_kExprList.push_back(cur_kExprList);
	for (auto cur_kExprList : v_kExprList)
	{
		IR *node_kExpr = cur_kExprList->left_;
		s_column *cur_column = handle_kExpr(node_kExpr, p_cur_info);

		
		IR *node_kOptAsAlias = cur_kExprList->right_;
		if (node_kOptAsAlias->left_)
		{

			IR *node_alias = node_kOptAsAlias->left_->left_;
			string cur_alias = gen_id_name();
			node_alias->str_val_ = cur_alias;
			cur_column->alias = cur_alias;
		}
		if (cur_column->ColumnName || !cur_column->alias.empty())
		{
			p_cur_info->selectTarget_column.push_back(cur_column);
		}
	}
}


s_column *handle_kExpr(IR *node_kExpr, select_field_info *p_cur_info)
{

	s_column *ret_column;
	
	
	if (node_kExpr->left_->left_->left_ == NULL)
	{
		ret_column = new s_column;
		g_all_column.push_back(ret_column);
	}
	else if (node_kExpr->left_->left_->left_->type_ == kExprColumnName)
	{
		IR *node_kExprColumnName = node_kExpr->left_->left_->left_;
		ret_column = handle_kExprColumnName(node_kExprColumnName);
	}
	else
	{
		ret_column = new s_column;
		g_all_column.push_back(ret_column);

		if (node_kExpr->left_->left_->type_ == kFunctionExpr)
		{
			IR *node_kFunctionExpr = node_kExpr->left_->left_;
			IR *node_kOptOverClause = node_kFunctionExpr->right_;
			IR *function_name;

			if (node_kFunctionExpr->case_idx_ == CASE0)
			{
				function_name = node_kFunctionExpr->left_->left_;
			}
			if (node_kFunctionExpr->case_idx_ == CASE1)
			{
				IR *node_kExprList = node_kFunctionExpr->left_->right_;
				dfs_traverse_kExpr(node_kExprList, p_cur_info->function_column);
				function_name = node_kFunctionExpr->left_->left_->left_->left_;
			}
			
			function_name->str_val_ = vector_rand_ele_str(function_name_library);
			
			
			handle_kOptOverClause(node_kOptOverClause, p_cur_info);
			return ret_column;
		}
		
		else
		{
			dfs_traverse_kExpr(node_kExpr, p_cur_info->selectTarget_column);
		}
	}
	return ret_column;
}
void handle_kOptOverClause(IR *node_kOptOverClause, select_field_info *p_cur_info)
{
	
	
	if (node_kOptOverClause->case_idx_ == CASE0)
	{
		IR *node_kWindow = node_kOptOverClause->left_;
		handle_kWindow(node_kWindow, p_cur_info);
	}
	
	if (node_kOptOverClause->case_idx_ == CASE1)
	{

		IR *window_name = node_kOptOverClause->left_->left_;
		p_cur_info->window_name.push_back(window_name);
	}
	
}

void handle_kWindow(IR *node_kWindow, select_field_info *p_cur_info)
{
	IR *node_kOptExistWindowName = node_kWindow->left_->left_->left_;
	IR *window_name = node_kOptExistWindowName->left_;
	if (window_name != NULL)
	{
		p_cur_info->window_name.push_back(window_name);
	}

	IR *node_kOptPartition = node_kWindow->left_->left_->right_;
	dfs_traverse_kExpr(node_kOptPartition, p_cur_info->window_column);

	IR *node_kOptOrderClause = node_kWindow->left_->right_;
	dfs_traverse_kExpr(node_kOptOrderClause, p_cur_info->window_column);

	
	IR *node_kOptFrameClause = node_kWindow->right_;
	dfs_traverse_kExpr(node_kOptFrameClause, p_cur_info->window_column);
}

void handle_kColumnNameList(IR *node_kColumnNameList, vector<s_column *> &column)
{

	IR *node_kColumnName = node_kColumnNameList->left_;
	s_column *cur_column = handle_kColumnName(node_kColumnName);
	column.push_back(cur_column);

	while (node_kColumnNameList->right_)
	{

		node_kColumnNameList = node_kColumnNameList->right_;

		node_kColumnName = node_kColumnNameList->left_;
		s_column *cur_column = handle_kColumnName(node_kColumnName);
		column.push_back(cur_column);
	}
}
s_column *handle_kExprColumnName(IR *node_kExprColumnName)
{

	s_column *ret_column = new s_column;
	g_all_column.push_back(ret_column);

	if (node_kExprColumnName->right_)
	{
		ret_column->TableName = node_kExprColumnName->left_->left_;
		ret_column->ColumnName = node_kExprColumnName->right_->left_;
	}
	else
	{
		ret_column->ColumnName = node_kExprColumnName->left_->left_;
	}
	return ret_column;
}

s_column *handle_kColumnName(IR *node_kColumnName)
{

	s_column *ret_column = new s_column;
	g_all_column.push_back(ret_column);

	ret_column->ColumnName = node_kColumnName->left_;

	return ret_column;
}

IR *get_kOptTablePrefix(IR *cur_kTableRef)
{

	IR *cur_node = cur_kTableRef;

	while (cur_node)
	{

		cur_node = cur_node->left_;

		if (cur_node->type_ == kOptTablePrefix)
		{

			return cur_node;
		}
	}
}
void handle_kTableRef(IR *cur_kTableRef, vector<s_table *> &output_table)
{

	IR *cur_kOptTablePrefix = get_kOptTablePrefix(cur_kTableRef);

	vector<IR *> v_kTableRef;
	vector<IR *> v_kJoinOp;

	
	v_kTableRef.push_back(cur_kTableRef);
	while (cur_kOptTablePrefix->left_)
	{

		cur_kTableRef = cur_kOptTablePrefix->left_;
		IR *cur_kJoinOp = cur_kOptTablePrefix->right_;

		v_kTableRef.push_back(cur_kTableRef);
		v_kJoinOp.push_back(cur_kJoinOp);

		cur_kOptTablePrefix = get_kOptTablePrefix(cur_kTableRef);
	}

	IR *node_kTableRef = v_kTableRef.back();
	v_kTableRef.pop_back();

	
	s_table *cur_table = kTableRef_to_table(node_kTableRef);

	while (!v_kTableRef.empty())
	{

		IR *node_kJoinOp = v_kJoinOp.back();
		v_kJoinOp.pop_back();

		IR *node_kTableRef = v_kTableRef.back();
		v_kTableRef.pop_back();

		
		
		IR *node_kOptOn = node_kTableRef->left_->right_;
		IR *node_kOptUsing = node_kTableRef->right_;
		
		if (node_kJoinOp->case_idx_ == CASE0)
		{

			output_table.push_back(cur_table);
			cur_table = kTableRef_to_table(node_kTableRef);
			
			if (node_kOptOn->left_ || node_kOptUsing->left_)
			{
				g_return_val = false;
			}
		}
		
		else
		{
			s_table *cur_table_2 = kTableRef_to_table(node_kTableRef);
			cur_table = join_table(cur_table, cur_table_2);

			if (node_kOptOn->left_)
			{

				vector<s_column *> on_column;
				dfs_traverse_kExpr(node_kOptOn->left_, on_column);
				build_kOptOn_graph(cur_table, on_column);
			}
			
			if (node_kOptUsing->left_)
			{

				vector<s_column *> using_column;
				handle_kColumnNameList(node_kOptUsing->left_, using_column);
				build_kOptUsing_graph(cur_table, using_column);
			}
		}
	}
	
	
	output_table.push_back(cur_table);
}


void handle_kOptFromClause(IR *node_kOptFromClause, vector<s_table *> &from_table)
{
	if (!node_kOptFromClause->left_)
	{
		return;
	}

	IR *cur_kFromClause = node_kOptFromClause->left_;
	IR *cur_kTableRef = cur_kFromClause->left_;
	handle_kTableRef(cur_kTableRef, from_table);
}

s_table *kTableRef_to_table(IR *node_kTableRef)
{
	
	s_table *ret_table = new s_table;
	g_all_table.push_back(ret_table);

	IR *node_kOptAsAlias;

	
	if (node_kTableRef->case_idx_ == CASE0)
	{

		IR *cur_kTableName = node_kTableRef->left_->left_->left_->left_->right_;
		ret_table->v_TableName.push_back(cur_kTableName->left_);

		node_kOptAsAlias = node_kTableRef->left_->left_->left_->right_;
	}
	
	if (node_kTableRef->case_idx_ == CASE1)
	{

		IR *cur_kSelectNoParens = node_kTableRef->left_->left_->left_->right_;
		ret_table = handle_kSelectNoParens(cur_kSelectNoParens);

		node_kOptAsAlias = node_kTableRef->left_->left_->right_;
		
	}
	
	if (node_kTableRef->case_idx_ == CASE2)
	{

		IR *cur_kTableRef = node_kTableRef->left_->left_->left_->right_;
		ret_table = handle_kTableRef(cur_kTableRef);

		node_kOptAsAlias = node_kTableRef->left_->left_->right_;
	}
	
	if (node_kOptAsAlias->left_)
	{

		IR *node_alias = node_kOptAsAlias->left_->left_;
		string cur_alias = gen_id_name();
		node_alias->str_val_ = cur_alias;
		ret_table->alias = cur_alias;
	}
	

	

	return ret_table;
}

s_table *join_table(s_table *table_1, s_table *table_2)
{

	s_table *ret_table = new s_table;
	g_all_table.push_back(ret_table);

	ret_table->v_TableName.insert(ret_table->v_TableName.end(), table_1->v_TableName.begin(), table_1->v_TableName.end());
	ret_table->v_TableName.insert(ret_table->v_TableName.end(), table_2->v_TableName.begin(), table_2->v_TableName.end());

	ret_table->v_ColumnName.insert(ret_table->v_ColumnName.end(), table_1->v_ColumnName.begin(), table_1->v_ColumnName.end());
	ret_table->v_ColumnName.insert(ret_table->v_ColumnName.end(), table_2->v_ColumnName.begin(), table_2->v_ColumnName.end());

	return ret_table;
}

s_table *handle_kTableRef(IR *cur_kTableRef)
{

	IR *cur_kOptTablePrefix = get_kOptTablePrefix(cur_kTableRef);

	vector<IR *> v_kTableRef;
	vector<IR *> v_kJoinOp;

	v_kTableRef.push_back(cur_kTableRef);
	while (cur_kOptTablePrefix->left_)
	{

		cur_kTableRef = cur_kOptTablePrefix->left_;
		IR *cur_kJoinOp = cur_kOptTablePrefix->right_;

		v_kTableRef.push_back(cur_kTableRef);
		v_kJoinOp.push_back(cur_kJoinOp);

		cur_kOptTablePrefix = get_kOptTablePrefix(cur_kTableRef);
	}

	IR *node_kTableRef = v_kTableRef.back();
	v_kTableRef.pop_back();

	s_table *cur_table = kTableRef_to_table(node_kTableRef);

	while (!v_kTableRef.empty())
	{

		IR *node_kJoinOp = v_kJoinOp.back();
		v_kJoinOp.pop_back();

		IR *node_kTableRef = v_kTableRef.back();
		v_kTableRef.pop_back();
		
		
		if (node_kJoinOp->case_idx_ == CASE0)
		{

			
		}
		
		else
		{

			s_table *cur_table_2 = kTableRef_to_table(node_kTableRef);
			cur_table = join_table(cur_table, cur_table_2);
		}
	}
	return cur_table;
}

s_table *columns_to_table(vector<s_column *> &selectTarget_column)
{

	s_table *ret_table = new s_table;
	g_all_table.push_back(ret_table);

	for (int i = 0; i < selectTarget_column.size(); i++)
	{

		ret_table->v_ColumnName.push_back(selectTarget_column[i]);
	}
	return ret_table;
}

void validate_build_graph(select_field_info *p_cur_info)
{
	vector<s_column *> column_depend_on_from;
	
	
	

	
	column_depend_on_from.insert(column_depend_on_from.end(), p_cur_info->selectTarget_column.begin(), p_cur_info->selectTarget_column.end());
	column_depend_on_from.insert(column_depend_on_from.end(), p_cur_info->where_column.begin(), p_cur_info->where_column.end());
	column_depend_on_from.insert(column_depend_on_from.end(), p_cur_info->group_by_column.begin(), p_cur_info->group_by_column.end());
	column_depend_on_from.insert(column_depend_on_from.end(), p_cur_info->order_by_column.begin(), p_cur_info->order_by_column.end());
	column_depend_on_from.insert(column_depend_on_from.end(), p_cur_info->function_column.begin(), p_cur_info->function_column.end());
	column_depend_on_from.insert(column_depend_on_from.end(), p_cur_info->window_column.begin(), p_cur_info->window_column.end());

	if ((column_depend_on_from.size() != 0) && (p_cur_info->from_table.size() == 0))
	{
		g_return_val = false;
		
		return;
	}

	
	if (p_cur_info->with_table.size() != 0)
	{

		for (auto cur_table : p_cur_info->from_table)
		{

			for (auto iter = cur_table->v_TableName.begin(); iter != cur_table->v_TableName.end();)
			{

				int rand_with_table_index = rand() % (p_cur_info->with_table.size() + g_table_names.size());
				if (rand_with_table_index < p_cur_info->with_table.size())
				{
					
					
					s_table *cur_with_table = p_cur_info->with_table[rand_with_table_index];
					(*iter)->str_val_ = cur_with_table->alias;
					
					cur_table->v_ColumnName.insert(cur_table->v_ColumnName.end(), cur_with_table->v_ColumnName.begin(), cur_with_table->v_ColumnName.end());
					
					iter = cur_table->v_TableName.erase(iter);
				}
				else
				{
					++iter;
				}
			}
		}
	}
	
	for (auto cur_column : column_depend_on_from)
	{
		
		if (cur_column->ColumnName)
		{
			s_table *pick_table = vector_rand_ele(p_cur_info->from_table);
			column_to_certain_table(cur_column, pick_table);
		}
	}

	
	for (auto table : p_cur_info->from_table)
	{
		for (auto IR_table : table->v_TableName)
		{
			if (find(g_used_table.begin(), g_used_table.end(), IR_table) == g_used_table.end())
			{
				g_stmt_graph.not_used_table.push_back(IR_table);
			}
		}
	}
}

void column_to_certain_table(s_column *cur_column, s_table *pick_table)
{

	certain_table *c_table = new certain_table;
	g_all_certain_table.push_back(c_table);
	c_table->alias = pick_table->alias;

	if (!pick_table->v_ColumnName.empty() && !pick_table->v_TableName.empty())
	{
		int rand_table_index = rand() % (pick_table->v_TableName.size() + 1);
		
		if (rand_table_index == 0)
		{
			c_table->rely_column = vector_rand_ele(pick_table->v_ColumnName);
			g_stmt_graph.column_to_column[cur_column] = c_table;
		}
		
		else
		{
			IR *cur_table = pick_table->v_TableName[rand_table_index - 1];
			c_table->rely_table = cur_table;
			g_stmt_graph.column_to_table[cur_column] = c_table;
			g_used_table.insert(cur_table);
		}
	}

	else if (!pick_table->v_ColumnName.empty() && pick_table->v_TableName.empty())
	{
		c_table->rely_column = vector_rand_ele(pick_table->v_ColumnName);
		g_stmt_graph.column_to_column[cur_column] = c_table;
	}

	else if (pick_table->v_ColumnName.empty() && !pick_table->v_TableName.empty())
	{
		IR *cur_table = vector_rand_ele(pick_table->v_TableName);
		c_table->rely_table = cur_table;
		g_stmt_graph.column_to_table[cur_column] = c_table;
		g_used_table.insert(cur_table);
	}
	
	else
	{
		
		g_return_val = false;
		
		return;
	}
}

void build_kOptOn_graph(s_table *pick_table, vector<s_column *> &on_column)
{

	for (auto cur_column : on_column)
	{

		column_to_certain_table(cur_column, pick_table);
	}
}
void build_kOptUsing_graph(s_table *pick_table, vector<s_column *> &using_column)
{

	for (auto cur_column : using_column)
	{

		column_to_certain_table(cur_column, pick_table);
	}
}


void handle_kOptWhereClause(IR *node_kOptWhereClause, vector<s_column *> &where_column)
{

	dfs_traverse_kExpr(node_kOptWhereClause, where_column);
}


void handle_kOptGroupClause(IR *node_kOptGroupClause, vector<s_column *> &group_by_column)
{

	dfs_traverse_kExpr(node_kOptGroupClause, group_by_column);
}


void handle_kOptOrderClause(IR *node_kOptOrderClause, vector<s_column *> &order_by_column)
{

	dfs_traverse_kExpr(node_kOptOrderClause, order_by_column);
}

void dfs_traverse_kExpr(IR *node, vector<s_column *> &output_column)
{

	if (node->type_ == kSelectNoParens)
	{
		handle_kSelectNoParens(node);
		return;
	}

	if (node->type_ == kFunctionName)
	{
		node->left_->str_val_ = vector_rand_ele(function_name_library);
		return;
	}

	if (node->type_ == kIntLiteral)
	{
		if (get_rand_int(100) > 50)
			node->int_val_ = vector_rand_ele(value_library);
		else
			node->int_val_ = get_rand_int(100);
		return;
	}

	if (node->type_ == kFloatLiteral)
	{
		node->float_val_ = (double)(get_rand_int(100000000));
		return;
	}

	if (node->type_ == kExprColumnName)
	{
		s_column *cur_column = handle_kExprColumnName(node);
		output_column.push_back(cur_column);
		return;
	}

	if (node->left_)
	{
		dfs_traverse_kExpr(node->left_, output_column);
	}
	if (node->right_)
	{
		dfs_traverse_kExpr(node->right_, output_column);
	}
}

vector<s_table *> hanle_kOptWithClause(IR *node_kOptWithClause)
{

	vector<s_table *> with_table;

	if (!node_kOptWithClause->left_)
	{

		return with_table;
	}
	IR *node_kCteTableList = node_kOptWithClause->left_;

	vector<IR *> v_kCteTable;
	v_kCteTable.push_back(node_kCteTableList->left_);

	while (node_kCteTableList->right_)
	{

		node_kCteTableList = node_kCteTableList->right_;
		v_kCteTable.push_back(node_kCteTableList->left_);
	}

	for (auto node_kCteTable : v_kCteTable)
	{

		s_table *cur_with_table = handle_kCteTable(node_kCteTable);
		
		if (cur_with_table->v_ColumnName.size() != 0 || cur_with_table->v_TableName.size() != 0)
		{
			with_table.push_back(cur_with_table);
		}
	}
	return with_table;
}

s_table *handle_kCteTable(IR *node_kCteTable)
{

	IR *node_kSelectStmt = node_kCteTable->right_;
	s_table *ret_table = handle_kSelectStmt(node_kSelectStmt);

	IR *table_name = node_kCteTable->left_->left_->left_;
	string cur_table_name = gen_id_name();
	table_name->str_val_ = cur_table_name;
	ret_table->alias = cur_table_name;
	IR *node_kOptColumnNameListP = node_kCteTable->left_->right_;
	if (node_kOptColumnNameListP->left_)
	{
		IR *node_kOptColumnNameList = node_kOptColumnNameListP->left_;

		vector<IR *> v_column_name;
		v_column_name.push_back(node_kOptColumnNameList->left_->left_);
		while (node_kOptColumnNameList->right_)
		{

			node_kOptColumnNameList = node_kOptColumnNameList->right_;
			v_column_name.push_back(node_kOptColumnNameList->left_->left_);
		}
		
		if (v_column_name.size() != ret_table->v_ColumnName.size())
		{
			g_return_val = false;
			
			return ret_table;
		}
		for (int i = 0; i < v_column_name.size(); i++)
		{

			string cur_column_name = gen_id_name();
			v_column_name[i]->str_val_ = cur_column_name;
			ret_table->v_ColumnName[i]->alias = cur_column_name;
		}
	}
	return ret_table;
}
