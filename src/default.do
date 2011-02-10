case $1 in
  *.o)
    CPP_SOURCE="${1%.o}.cpp" # default.do doesn't strip extension
    redo-ifchange $CPP_SOURCE
    g++ -O2 -MMD -MF $3.deps.tmp -c -o $3 $CPP_SOURCE
    DEPS=$(sed -e "s|^$3:||" -e 's/\\//g' <$3.deps.tmp)
    for dep in $DEPS; do
      redo-ifchange $dep
    done
    rm -f $3.deps.tmp
#    redo-ifchange $DEPS TODO: work out how this triggers a redo bug
    ;;
  Loki2)
    DEPS=$(find -name "*.cpp" | sed -e s/\.cpp/\.o/)
    redo-ifchange $DEPS
    g++ -L/usr/local/lib-linux64 -o $3 $DEPS -lsystemc
    ;;
  *) echo "no rule to build '$1'" >&2; exit 1 ;;
esac
