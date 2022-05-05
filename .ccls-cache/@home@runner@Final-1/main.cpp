#include <iostream>
#include "SQLiteDb.hpp"

int main() {
  SQLiteDb db;
  db.connect("./main.db");
  std::cerr << db.execStatement(".print hello world") 
            << db.execStatement(".print goodbye world");
}