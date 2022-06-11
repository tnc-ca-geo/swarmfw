/*
 * This file contains helper functions used in different places of the project
 * that might be packaged up in a library later
 *
 * The inline key word allows for using these in different source files at the
 * same time, see:
 *
 * https://stackoverflow.com/questions/12981725/include-guards-not-working
 */
/*
 * We are using some Arduino specific types here such as boolean and size_t
 * TODO: remove?
 */
#include <Arduino.h>
#ifndef _MY_HELPERS_H_
#define _MY_HELPERS_H_


namespace helpers {
  /*
   * Push a bfr on a stack. Make sure it gets inserted after the last complete
   * message on the stack. Return false if message cannot be added.
   */
  bool pushToStack(
    char* stack, const char* bfr, const size_t len, const size_t stackSize,
    const char sep='\n');

  /*
   * Search a buffer and return index of find, returns -1 if not found. Return
   * also -1 when srchLen=0
   */
  int16_t searchBfr(
    const char *bfr, const size_t len, const char *srchTrm, const size_t srchLen
  );

  /*
   * Take the first part of a buffer separated by sep, update the original
   * buffer return the part len.
   * TODO: use build-in functions?
   */
  size_t popFromStack(
    char* part, char *bfr, const size_t bfrSize, const char sep='\n');
}

#endif /* _MY_HELPERS_H_ */
