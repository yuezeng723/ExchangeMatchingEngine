#include "exercise.h"

void exercise(connection *C)
{
  cout << "do create" << endl;
  doCreate(C, 0, 100, "sy", 10);
  cout << endl;

  cout << "do order" << endl;
  doOrder(C, 0, 0, "sy", -10, 12);
  cout << endl;

  cout << "do query" << endl;
  doQueryOpen(C, 0);
  doQueryExecute(C, 0);
  doQueryCancel(C, 0);
  cout << endl;

  cout << "do cancel" << endl;
  doCancel(C, 0, 0);
  cout << endl;
}
