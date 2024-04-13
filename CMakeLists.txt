project(test-rbtree)

add_executable(test-rbtree rbtree.h test-utils.h test-rbtree.cc)

target_link_libraries(
    test-rbtree
  PRIVATE
    Catch2::Catch2WithMain
)
target_compile_features(test-rbtree PUBLIC cxx_std_17)

