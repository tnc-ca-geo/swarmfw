#include "myHelpers.h"
using namespace helpers;


bool helpers::pushToStack(
  char* stack, const char* bfr, const size_t len, const size_t stackSize,
  const char sep
) {
  size_t pos = strlen(stack);
  size_t actualLen = len;
  int16_t sepPos = -1;
  int16_t startPos = 0;
  // return false if new string does not fit into stackSize
  if (pos + len > stackSize - 2) { return false; }
  // remove trailing \n {
  while (bfr[actualLen-1] == 10 && actualLen > 0) { actualLen--; }
  // there seems to be no reverse equivalent to strstr at least not without
  // using std::str class which we don't
  for (sepPos=pos; sepPos>-1; sepPos--) {
    if (stack[sepPos] == sep) { break; }
  }
  // start at position 0 if |n not found
  if (sepPos > -1) { startPos = sepPos; };
  // include a character for \n
  memmove(stack+startPos+actualLen+1, stack+startPos, pos-startPos);
  stack[startPos+actualLen] = '\n';
  memmove(stack+sepPos+1, bfr, actualLen);
  stack[pos+actualLen+1] = '\0';
  return true;
};


int16_t helpers::searchBfr(
  const char *bfr, const size_t len, const char *srchTrm, const size_t srchLen
) {
  // do not search if srchTrm is NULL pointer
  if (!srchLen) { return -1; }
  // ensure that the string is really \0 terminated
  char lin[len+1] = {0};
  char srch[srchLen+1] = {0};
  memcpy(lin, bfr, len);
  memcpy(srch, srchTrm, srchLen);
  char *res = strstr(lin, srch);
  if (res) { return res - lin; } else { return -1; }
}


size_t helpers::popFromStack(
  char* part, char *bfr, const size_t bfrSize, const char sep
) {
  int16_t pos = searchBfr(bfr, bfrSize, "\n", 1);
  if (pos > -1) {
    memcpy(part, bfr, pos);
    memmove(bfr, bfr+pos+1, bfrSize-pos);
    bfr[bfrSize-pos-1] = '\0';
    part[pos] = '\0';
    return pos;
  }
  part[0] = '\0';
  return 0;
};
