#define _CRT_SECURE_NO_WARNINGS
#define MAXVAL 1e9
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include "mysql.h"
using namespace std;

#pragma comment(lib, "libmysql.lib")

const char* host = "localhost";
const char* user = "root";
const char* pw = "20181650ehgus";
const char* db = "project";
string pastyear = "2021";	//작년 변수
string year_for_pastmonth = "2022";	//1월일 경우 지난달을 위해 작년의 년도 저장
string month_for_pastmonth = "5";	//지난 달 저장

//selection message를 출력하고 stdin에서 입력받아 그 값을 리턴하는 함수
int putSelectMessage() {	
	int input;
	cout << "\n---------- SELECT QUERY TYPES ----------\n\n";
	cout << "\t1. TYPE 1\n" << "\t2. TYPE 2\n" << "\t3. TYPE 3\n" << "\t4. TYPE 4\n" << "\t5. TYPE 5\n"
		<< "\t6. TYPE 6\n" << "\t7. TYPE 7\n" << "\t0. QUIT\n\n";
	cout << "USER INPUT: ";
	cin >> input;
	return input;
}
//sql_result로부터 쿼리 결과를 stdout에 출력. 두번째 인자는 그 출력할 튜플의 개수로, 디폴트 인자로 큰 수 매크로를 준다.
void query_print(MYSQL_RES* sql_result, int n=MAXVAL) {
	MYSQL_ROW sql_row;
	int num_fields;
	num_fields = mysql_num_fields(sql_result);	//필드(속성)의 개수
	while ((sql_row = mysql_fetch_row(sql_result)) != NULL && n > 0)
	{
		n--;
		for (int i = 0; i < num_fields; i++) {
			if (sql_row[i])
				cout << sql_row[i] << " | ";
			else
				cout << "NULL" << " | ";	//속성에 NULL이 들어갈 경우 출력을 따로 하여 오류를 예방
		}
		cout << '\n';
	}
	mysql_free_result(sql_result);
}
int main(void) {
	//BASIC CONNECTION ROUTINE
	MYSQL* connection = NULL;
	MYSQL conn;
	MYSQL_RES* sql_result;
	MYSQL_ROW sql_row;
	char* temp = new char[10];
	if (mysql_init(&conn) == NULL)
		cout << "mysql_init() error!";
	connection = mysql_real_connect(&conn, host, user, pw, db, 3306, (const char*)NULL, 0);
	if (connection == NULL)
	{
		printf("%d ERROR : %s\n", mysql_errno(&conn), mysql_error(&conn));
		return 1;
	}

	cout << "Connection Succeed" << '\n';
	if (mysql_select_db(&conn, db))
	{
		printf("%d ERROR : %s\n", mysql_errno(&conn), mysql_error(&conn));
		return 1;
	}
	//CONNECTION ROUTINE ENDS

	string buf;	//sql 처리를 위한 버퍼
	int state,num_fields;	//state=쿼리 요청 함수의 리턴값, num_fields=필드의 개수 저장
	ifstream fin("20181650_create_insert.txt");	//Create와 Insert가 쓰여 있는 입력파일 open
	if (fin.fail()) {
		cerr << "create&insert txt file open error!\n";
		return 1;
	}
	while (!fin.eof()) {
		getline(fin, buf,';');
		cout << buf;
		if (buf.empty())
			continue;
		state = mysql_query(connection, buf.c_str());
		if (state)
			printf("%d ERROR : %s\n", mysql_errno(&conn), mysql_error(&conn));
		buf.clear();
	}
	fin.close();

	//query routine starts
	while (1) {
		int input = putSelectMessage();	//selection message 반복 출력
		if (input == 0) {	//0 input: break-> delete/drop 진행
			cout << "---- User Requested QUIT! ----\n\n";
			break;
		}
		else if (input == 1) {	//1 input: tracking_num을 입력받아 그로부터 연결된 주문 정보로와 연결해 고객의 연락처를 받음.
			cout << "---- TYPE 1 ----\n\n";
			string x;
			cout << "Input tracking_num: ";
			cin >> x;
			//코드 전체에서 가장 복잡한 join. join이 무려 네번이나 이루어지지만 redundancy를 최소화한 디자인의 한계일 것이다.
			//필요한 정보인 고객 id와 이름, 연락처 속성만 표시해 가시성 제고
			buf = "select customer_id,customer_name,phone_num from ((shipping natural join online_order)inner join _order using (order_id)) inner join customer using (customer_id) where tracking_num = \'" + x + "\'";
			state = mysql_query(connection, buf.c_str());
			if (state) {
				cerr << "TYPE 1 Error!\n";
				printf("%d ERROR : %s\n", mysql_errno(&conn), mysql_error(&conn));
				continue;
			}
			cout << "customer_id | customer_name | contact\n";
			cout << "-------------------------------------\n";
			sql_result = mysql_store_result(connection);
			query_print(sql_result);

			int subinput;
			while (1) {
				cout << "----------Subtypes in TYPE 1 ----------\n\n";
				cout << "\t1. TYPE 1-1\n\t0. Back to Select menu\n\n";
				cout << "USER INPUT: ";
				cin >> subinput;
				//1-1 input: 1에서 입력받은 배송 정보를 tracking_num만 바꾸어 shipping 테이블에 갱신, 또
				//문제가 생긴 해당 주문의 tracking_num을 새로 갱신한다.
				if (subinput == 1) { 
					cout << "-----TYPE 1-1 -----\n\n";
					buf = "select tracking_num from shipping";	//shipping 테이블의 튜플의 개수를 계산하기 위한 쿼리
					state = mysql_query(connection, buf.c_str());
					sql_result = mysql_store_result(connection);
					sql_row = mysql_fetch_row(sql_result);
					int maxship = 100000 + mysql_num_rows(sql_result);	//base tracking_num=100000 + row개수 = maxship
					maxship++;	//새 운송 정보에 부여할 tracking_num.
					_itoa(maxship, temp, 10);
					string tmp = temp;

					buf = "select * from shipping where tracking_num=\'" + x + "\'";	//1에서 입력받은 넘버로 운송정보 조회
					state = mysql_query(connection, buf.c_str());
					sql_result = mysql_store_result(connection);
					sql_row = mysql_fetch_row(sql_result);
					cout << "tracking_num | company | address | isarrived | got_problem\n";
					cout << "----------------------------------------------------------\n";
					cout << sql_row[0] << " | " << sql_row[1] << " | " << sql_row[2] << " | " << sql_row[3] << " | " << sql_row[4] << '\n';
					mysql_free_result(sql_result);

					//새롭게 갱신한 운송정보 삽입
					buf = "insert into shipping values(" + tmp + ",\'" + sql_row[1] + "\',\'" + sql_row[2] + "\'," + "0,0);";
					cout << "\nNow processing " << buf << '\n';
					state = mysql_query(connection, buf.c_str());
					if (state) {
						cerr << "TYPE 1-1 Error!\n";
						printf("%d ERROR : %s\n", mysql_errno(&conn), mysql_error(&conn));
					}

					//주문 정보에서 기존의 문제가 생긴 tracking_num 자리에 새로운 넘버를 갱신한다.
					buf = "update online_order set tracking_num= " + tmp + " where tracking_num=" + x;
					cout << "\nNow processing " << buf << '\n';
					state = mysql_query(connection, buf.c_str());
					if (state) {
						cerr << "TYPE 1-1 Error!\n";
						printf("%d ERROR : %s\n", mysql_errno(&conn), mysql_error(&conn));
					}
				}
				else if(subinput==0) {	//back to selection menu
					break;
				}
			}
		}
		else if (input == 2) {	//2 input: 작년에 가격 기준 가장 많이 구매한 고객 정보를 받음.
			cout << "---- TYPE 2 ----\n\n";
			//with절을 사용해 작년 구매 정보를 먼저 받고, 그로부터 max값을 얻도록 sql문 작성
			buf = "with max_buy_price as (select customer_id, customer_name, date_year, sum(price * num_of_order) as pricesum from(_order natural join customer) inner join product using(product_id)";
			buf = buf + "where date_year = " + pastyear + " group by customer_id) select customer_id, customer_name, pricesum from max_buy_price where pricesum = (select max(pricesum) from max_buy_price);";
			state = mysql_query(connection, buf.c_str());
			if (state) {
				cerr << "TYPE 2 Error!\n";
				printf("%d ERROR : %s\n", mysql_errno(&conn), mysql_error(&conn));
				continue;
			}
			cout << "customer_id | customer_name | pricesum\n";
			cout << "-------------------------------------\n";
			sql_result = mysql_store_result(connection);
			sql_row = mysql_fetch_row(sql_result);
			cout << sql_row[0] << " | " << sql_row[1] << " | " << sql_row[2];
			string bestmanid = sql_row[0];	//subquery에서 사용할 vip고객의 id정보 별도 저장
			mysql_free_result(sql_result);

			int subinput;
			while (1) {
				cout << "\n\n----------Subtypes in TYPE 2 ----------\n\n";
				cout << "\t1. TYPE 2-1\n\t0. Back to Select menu\n\n";
				cout << "USER INPUT: ";
				cin >> subinput;
				if (subinput == 1) {
					cout << "-----TYPE 2-1 -----\n\n";
					//해당 고객이 개수 기준 가장 많이 산 상품을 with절을 사용해 두번에 걸쳐 얻는다.
					buf = "with max_buy_unit as(select product_id, product_name, _type, price, manufacturer, date_year, sum(num_of_order) as sum_order from(_order natural join customer) inner join product using(product_id)";
					buf = buf + " where customer_id = " + bestmanid + " and date_year = " + pastyear + " group by product_id) select * from max_buy_unit where sum_order = (select max(sum_order) from max_buy_unit); ";
					cout << "product_id | product_name | type | price | manufacturer | date_year | num_of_order\n";
					cout << "---------------------------------------------------------------------------------\n";
					state = mysql_query(connection, buf.c_str());
					sql_result = mysql_store_result(connection);
					query_print(sql_result);
				}
				else if (subinput == 0)
					break;
			}
		}
		else if (input == 3) {
		//주문 정보, 고객 정보, 상품 정보를 join하여 작년에 주문된 기록이 있는 주문 정보를 view에 저장한다.
		buf = "create view pastyearsell as select * from _order natural join customer natural join product where date_year = " + pastyear + ";";
			state = mysql_query(connection, buf.c_str());
			if (state) {
				cerr << "View generating error before TYPE3!\n";
				printf("%d ERROR : %s\n", mysql_errno(&conn), mysql_error(&conn));
				continue;
			}
			cout << "---- TYPE 3 ----\n\n";
			//팔린 금액을 sum한 뒤 product_id별로 묶어 해당하는 속성들을 출력한다.
			buf = "select product_id, product_name, _type, price, manufacturer,sum(num_of_order*price) as sold_total from pastyearsell group by product_id order by sold_total desc;";
			state = mysql_query(connection, buf.c_str());
			if (state) {
				cerr << "TYPE 3 Error!\n";
				printf("%d ERROR : %s\n", mysql_errno(&conn), mysql_error(&conn));
				continue;
			}
			cout << "product_id | product_name | type | price | manufacturer | sold_total\n";
			cout << "--------------------------------------------------------------------\n";
			sql_result = mysql_store_result(connection);
			query_print(sql_result);

			int subinput;
			while (1) {
				cout << "\n\n----------Subtypes in TYPE 3 ----------\n\n";
				cout << "\t1. TYPE 3-1\n\t2. TYPE 3-2\n\t0. Back to Select menu\n\n";
				cout << "USER INPUT: ";
				cin >> subinput;
				if (subinput == 1) {	//k를 입력받아 3의 결과에서 top k 튜플을 뽑아 출력한다.
					cout << "-----TYPE 3-1 -----\n\n";
					cout << " Which K ? : ";
					cin >> subinput;
					buf = "select product_id, product_name, _type, price, manufacturer,sum(num_of_order*price) as sold_total from pastyearsell group by product_id order by sold_total desc;";
					state = mysql_query(connection, buf.c_str());
					sql_result = mysql_store_result(connection);
					cout << "product_id | product_name | type | price | manufacturer | sold_total\n";
					cout << "--------------------------------------------------------------------\n";
					query_print(sql_result, subinput);
				}
				else if (subinput == 2) {	//3의 결과에서 top 10%의 튜플을 출력하기 위해 row개수/10을 연산 후 그만큼 출력한다.
					cout << "-----TYPE 3-2 -----\n\n";
					buf = "select product_id, product_name, _type, price, manufacturer,sum(num_of_order*price) as sold_total from pastyearsell group by product_id order by sold_total desc;";
					state = mysql_query(connection, buf.c_str());
					sql_result = mysql_store_result(connection);
					int topten = (int)mysql_num_rows(sql_result) / 10;
					cout << "product_id | product_name | type | price | manufacturer | sold_total\n";
					cout << "--------------------------------------------------------------------\n";
					query_print(sql_result, topten);
				}
				else if (subinput == 0)
					break;
			}

			buf = "DROP VIEW pastyearsell";	//view 제거
			state = mysql_query(connection, buf.c_str());
		}
		else if (input == 4) {	//3의 과정을 개수 기준으로 진행한다.
			//3에서와 같은 뷰 생성
			buf = "create view pastyearsell as select * from _order natural join customer natural join product where date_year = " + pastyear + ";";
			state = mysql_query(connection, buf.c_str());
			if (state) {
				cerr << "View generating error before TYPE4!\n";
				printf("%d ERROR : %s\n", mysql_errno(&conn), mysql_error(&conn));
				continue;
			}
			cout << "---- TYPE 4 ----\n\n";
			//3에서 가격과 개수를 곱했던 속성을 개수로만 작성해 쿼리 요청
			buf = "select product_id, product_name, _type, price, manufacturer,sum(num_of_order) as sold_total_unit from pastyearsell group by product_id order by sold_total_unit desc;";
			state = mysql_query(connection, buf.c_str());
			if (state) {
				cerr << "TYPE 4 Error!\n";
				printf("%d ERROR : %s\n", mysql_errno(&conn), mysql_error(&conn));
				continue;
			}
			cout << "product_id | product_name | type | price | manufacturer | sold_total_unit\n";
			cout << "-------------------------------------------------------------------------\n";
			sql_result = mysql_store_result(connection);
			query_print(sql_result);

			int subinput;
			while (1) {
				cout << "\n\n----------Subtypes in TYPE 4 ----------\n\n";
				cout << "\t1. TYPE 4-1\n\t2. TYPE 4-2\n\t0. Back to Select menu\n\n";
				cout << "USER INPUT: ";
				cin >> subinput;
				if (subinput == 1) {	//3와 동일하게 k 입력받아 top k 튜플 출력
					cout << "-----TYPE 4-1 -----\n\n";
					cout << " Which K ? : ";
					cin >> subinput;
					buf = "select product_id, product_name, _type, price, manufacturer,sum(num_of_order) as sold_total_unit from pastyearsell group by product_id order by sold_total_unit desc;";
					state = mysql_query(connection, buf.c_str());
					sql_result = mysql_store_result(connection);
					cout << "product_id | product_name | type | price | manufacturer | sold_total_unit\n";
					cout << "-------------------------------------------------------------------------\n";
					query_print(sql_result, subinput);
				}
				else if (subinput == 2) {	//top 10% 튜플 출력
					cout << "-----TYPE 4-2 -----\n\n";
					buf = "select product_id, product_name, _type, price, manufacturer,sum(num_of_order) as sold_total_unit from pastyearsell group by product_id order by sold_total_unit desc;";
					state = mysql_query(connection, buf.c_str());
					sql_result = mysql_store_result(connection);
					int topten = (int)mysql_num_rows(sql_result) / 10;
					cout << "product_id | product_name | type | price | manufacturer | sold_total_unit\n";
					cout << "-------------------------------------------------------------------------\n";
					query_print(sql_result, topten);
				}
				else if (subinput == 0)
					break;
			}

			buf = "DROP VIEW pastyearsell";	//뷰 삭제
			state = mysql_query(connection, buf.c_str());
		}
		else if (input == 5) {
			cout << "---- TYPE 5 ----\n\n";
			//product,storestock과 더불어 offlinestore도 join에 참여시켜 region과 재고 정보 속성을 다룰 수 있도록 함.
			//California만으로 조건을 준 뒤 product_id로 묶어 그 지역에서 sum(num_of_storestock)이 0이면 표시되도록 했다.
			buf = "select product_id,product_name,_type,manufacturer,region,sum(num_of_storestock) as sum_of_storestock from offlinestore natural join storestock natural join product where region = 'california' group by product_id having sum_of_storestock = 0;";
			state = mysql_query(connection, buf.c_str());
			if (state) {
				cerr << "TYPE 5 Error!\n";
				printf("%d ERROR : %s\n", mysql_errno(&conn), mysql_error(&conn));
				continue;
			}
			cout << "product_id | product_name | type | manufacturer | region | sum_of_storestock\n";
			cout << "----------------------------------------------------------------------------\n";
			sql_result = mysql_store_result(connection);
			query_print(sql_result);
		}
		else if (input == 6) {
			cout << "---- TYPE 6 ----\n\n";
			//got_problem이 set되어 있는(제때 도착하지 않은) shipping 정보를 출력
			buf = "select * from shipping where got_problem = 1;";
			state = mysql_query(connection, buf.c_str());
			if (state) {
				cerr << "TYPE 6 Error!\n";
				printf("%d ERROR : %s\n", mysql_errno(&conn), mysql_error(&conn));
				continue;
			}
			cout << "tracking_num | company | address | isarrived | got_problem\n";
			cout << "----------------------------------------------------------\n";
			sql_result = mysql_store_result(connection);
			query_print(sql_result);
		}
		else if (input == 7) {
			//지난 달에 팔린 주문 정보와 그 주문에 관계된 고객 정보를 뷰에 저장
			buf = "create view pastmonthsell as select * from _order natural join customer natural join product where date_year = " + year_for_pastmonth + " and date_mon = " + month_for_pastmonth + " ;";
			state = mysql_query(connection, buf.c_str());
			if (state) {
				cerr << "View generating error before TYPE4!\n";
				printf("%d ERROR : %s\n", mysql_errno(&conn), mysql_error(&conn));
				continue;
			}
			cout << "---- TYPE 7 ----\n\n";
			//bill은 contracted customer(account num is not null)에 대해서만 적용하므로, 해당 조건을 걸어준다.
			buf = "select customer_id,customer_name,order_id,product_name,price,num_of_order,price*num_of_order as total_purchase from pastmonthsell where account_num is not null";
			state = mysql_query(connection, buf.c_str());
			if (state) {
				cerr << "TYPE 7 Error!\n";
				printf("%d ERROR : %s\n", mysql_errno(&conn), mysql_error(&conn));
				continue;
			}
			sql_result = mysql_store_result(connection);
			int n_contracted = (int)mysql_num_rows(sql_result);	//bill을 만들 튜플의 개수
			while ((sql_row = mysql_fetch_row(sql_result)) != NULL) {
				cout << "-------------------------------------------\n";
				cout << "|\tcustomer_id: " << sql_row[0] << '\n';
				cout << "|\tcustomer_name: " << sql_row[1] << '\n';
				cout << "|\torder_id: " << sql_row[2] << '\n';
				cout << "|\tproduct_name: " << sql_row[3] << '\n';
				cout << "|\tprice: " << sql_row[4] << '\n';
				cout << "|\tnum_of_order: " << sql_row[5] << '\n';
				cout << "|\ttotal_price: " << sql_row[6] << '\n';
				cout << "-------------------------------------------\n\n";
			}
			mysql_free_result(sql_result);

			buf = "DROP VIEW pastmonthsell";	//뷰 제거
			state = mysql_query(connection, buf.c_str());
			continue;
		}
		else {
			cout << "You have to input values between 0 and 7\n";
		}
	}
	//query routine ends


	fin.open("20181650_delete_drop.txt");	//delete와 drop이 쓰여 있는 입력파일 open
	if (fin.fail()) {
		cerr << "delete&drop txt file open error!\n";
		return 1;
	}
	while (!fin.eof()) {
		getline(fin, buf, ';');
		cout << buf;
		if (buf.empty())
			continue;
		state = mysql_query(connection, buf.c_str());
		if (state)
			printf("%d ERROR : %s\n", mysql_errno(&conn), mysql_error(&conn));
		buf.clear();
	}
	fin.close();
	delete[] temp;
	mysql_close(connection);
	return 0;
}