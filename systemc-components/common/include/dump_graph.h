/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DUMP_GRAPH_H
#define DUMP_GRAPH_H

namespace gs {

class DumpGraphSingleton
{
public:
    static DumpGraphSingleton* GetInstance();
    void add_binding(std::string from, std::string to);
    void dump(std::string fname);

private:
    std::map<std::string, std::string> graph;
    std::map<std::string, int> component_ids;
    int nr_ids = 0;
    static const int COLAPSE_NUMBERED_NODES_DEPTH = 2;
    static DumpGraphSingleton* pDumpGraphSingleton;

    void colapse_numbered_nodes(void);
    void dump_nodes(std::string &buf);
    void dump_node(std::string &buf, std::string full_path);
    void dump_edges(std::string &buf);
    void generate_graph(std::string graph_str, std::string outfile);
};

} // namespace gs

#endif // DUMP_GRAPH_H
