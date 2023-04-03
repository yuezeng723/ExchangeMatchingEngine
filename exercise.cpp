#include "exercise.h"

void exercise(connection *C)
{
  cout << "do create" << endl;
  doCreate(C, 0, 100, "sy", 10);
  doCreate(C, 1, 100, "sz", 10);
  cout << endl;

  cout << "do order" << endl;
  doOrder(C, 0, 0, "sy", -10, 12);
  doOrder(C, 1, 0, "sz", 6, 11);
  doOrder(C, 2, 1, "sz", -5, 10);
  cout << endl;

  cout << "do open query" << endl;
  doQueryOpen(C, 0);
  doQueryOpen(C, 1);
  doQueryOpen(C, 2);
   cout << "do execute query" << endl;
  doQueryExecute(C, 0); 
  doQueryExecute(C, 1);
  doQueryExecute(C, 2);
  
  cout << "do cancel" << endl;
  doCancel(C, 0, 0);
  cout << endl;
  
  cout << "do cancel query" << endl;
  doQueryCancel(C, 0);
  doQueryCancel(C, 1);
  doQueryCancel(C, 2);
  cout << endl;

}
