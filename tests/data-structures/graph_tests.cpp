/**
 * \file data-structures/graph_tests.cpp
 **/
#include "graph_tests.hpp"

namespace other {

  TEST_F(graph_tests, test_add_nodes_and_edges) {
    graph<int> g;
    natural_t n0 = g.add_node(1);
    natural_t n1 = g.add_node(2);
    natural_t n2 = g.add_node(3);
    ASSERT_EQ(g.size(), 3);

    g.add_edge(n0, n1);
    g.add_edge(n1, n2);

    CORE_LOG_DEBUG("Graph:{}", g.to_matrix_string());
    CORE_LOG_DEBUG("Graph:\n{}", g.to_string());

    auto neighbors_of_1 = g.get_neighbors(n0);
    ASSERT_EQ(neighbors_of_1.size(), 1);
    EXPECT_EQ(neighbors_of_1[0], n1);

    auto neighbors_of_2 = g.get_neighbors(n1);
    ASSERT_EQ(neighbors_of_2.size(), 1);
    EXPECT_EQ(neighbors_of_2[0], n2);

    auto neighbors_of_3 = g.get_neighbors(n2);
    EXPECT_TRUE(neighbors_of_3.empty());

    g.add_edge(n0, n2);
    neighbors_of_1 = g.get_neighbors(n0);
    ASSERT_EQ(neighbors_of_1.size(), 2);
    EXPECT_TRUE((neighbors_of_1[0] == n1 && neighbors_of_1[1] == n2) || (neighbors_of_1[0] == n2 && neighbors_of_1[1] == n1));

    CORE_LOG_DEBUG("Graph:{}", g.to_matrix_string());
    CORE_LOG_DEBUG("Graph:\n{}", g.to_string());

    g.remove_edge(n0, n1);
    neighbors_of_1 = g.get_neighbors(n0);
    ASSERT_EQ(neighbors_of_1.size(), 1);
    EXPECT_EQ(neighbors_of_1[0], n2);

    g.remove_edge(n0, n2);
    neighbors_of_1 = g.get_neighbors(n0);
    ASSERT_TRUE(neighbors_of_1.empty());

    g.remove_node(n1);
    EXPECT_EQ(g.size(), 2);

    CORE_LOG_DEBUG("Graph:{}", g.to_matrix_string());
    CORE_LOG_DEBUG("Graph:\n{}", g.to_string());

    g.clear();
    EXPECT_TRUE(g.empty());
  }

  TEST_F(graph_tests, test_topological_sort) {
    graph<int> g;
    natural_t n0 = g.add_node(1);
    natural_t n1 = g.add_node(2);
    natural_t n2 = g.add_node(3);
    natural_t n3 = g.add_node(4);

    g.add_edge(n0, n1);
    g.add_edge(n0, n2);
    g.add_edge(n1, n3);
    g.add_edge(n2, n3);

    CORE_LOG_DEBUG("Graph:{}", g.to_matrix_string());
    CORE_LOG_DEBUG("Graph:\n{}", g.to_string());

    auto topo = g.topological_sort();
    EXPECT_FALSE(topo.has_cycles);
    EXPECT_EQ(topo.sorted_node_ids.size(), 4);
    EXPECT_TRUE((topo.sorted_node_ids[0] == n0 && topo.sorted_node_ids[3] == n3) || (topo.sorted_node_ids[0] == n0 && topo.sorted_node_ids[3] == n3));

    g.remove_node(n2);
    CORE_LOG_DEBUG("Graph:{}", g.to_matrix_string());
    CORE_LOG_DEBUG("Graph:\n{}", g.to_string());

    g.remove_node(n1);
    CORE_LOG_DEBUG("Graph:{}", g.to_matrix_string());
    CORE_LOG_DEBUG("Graph:\n{}", g.to_string());

    topo = g.topological_sort();
    EXPECT_FALSE(topo.has_cycles);
    EXPECT_EQ(topo.sorted_node_ids.size(), 2);
    EXPECT_TRUE((topo.sorted_node_ids[0] == n0 && topo.sorted_node_ids[1] == n3) || (topo.sorted_node_ids[0] == n3 && topo.sorted_node_ids[1] == n0));
  }

}  // namespace other