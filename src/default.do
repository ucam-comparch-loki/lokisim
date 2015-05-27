# Build script to be executed using 'redo'
# https://github.com/apenwarr/redo
case $1 in
  *.o)
    CPP_SOURCE="${1%.o}.cpp" # default.do doesn't strip extension
    redo-ifchange $CPP_SOURCE
    g++ -O2 -ffast-math -MMD -MF $3.deps.tmp -c -o $3 $CPP_SOURCE
    read DEPS <$3.deps.tmp
    rm -f $3.deps.tmp
#    redo-ifchange ${DEPS#*:} TODO: work out how this triggers a redo bug
    for dep in ${DEPS#*:}; do
      redo-ifchange $dep
    done
    ;;
  lokisim)
    DEPS=$(find -name "*.cpp" | sed -e s/\.cpp/\.o/)
    redo-ifchange $DEPS
    g++ -L/usr/local/lib-linux64 -o $3 $DEPS -lsystemc
    ;;
  *) echo "no rule to build '$1'" >&2; exit 1 ;;
esac
