dnl $Id$
dnl config.m4 for extension rotator

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(rotator, for rotator support,
dnl Make sure that the comment is aligned:
dnl [  --with-rotator             Include rotator support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(rotator, whether to enable rotator support,
dnl Make sure that the comment is aligned:
[  --enable-rotator           Enable rotator support])

if test "$PHP_ROTATOR" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-rotator -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/rotator.h"  # you most likely want to change this
  dnl if test -r $PHP_ROTATOR/$SEARCH_FOR; then # path given as parameter
  dnl   ROTATOR_DIR=$PHP_ROTATOR
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for rotator files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       ROTATOR_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$ROTATOR_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the rotator distribution])
  dnl fi

  dnl # --with-rotator -> add include path
  dnl PHP_ADD_INCLUDE($ROTATOR_DIR/include)

  dnl # --with-rotator -> check for lib and symbol presence
  dnl LIBNAME=rotator # you may want to change this
  dnl LIBSYMBOL=rotator # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $ROTATOR_DIR/lib, ROTATOR_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_ROTATORLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong rotator lib version or lib not found])
  dnl ],[
  dnl   -L$ROTATOR_DIR/lib -lm
  dnl ])
  dnl
  dnl PHP_SUBST(ROTATOR_SHARED_LIBADD)

  CFLAGS=-g -02
  CXXFLAGS=$CXXFLAGS

  PHP_REQUIRE_CXX()
  PHP_SUBST(ROTATOR_SHARED_LIBADD)
  PHP_ADD_LIBRARY(stdc++, 1, ROTATOR_SHARED_LIBADD)

  PHP_NEW_EXTENSION(rotator, rotator.cpp rotator_ctrl.cpp obj_timer.cpp, $ext_shared)
fi
