/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <iostream>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include <graphviz/gvc.h>
#include <dump_graph.h>

void gs::DumpGraphSingleton::add_binding(std::string from, std::string to)
{
    graph[from] = to;
}

void gs::DumpGraphSingleton::dump(std::string fname)
{
    this->colapse_numbered_nodes();
    std::string buf = "digraph dg {\ncompound=true;\n";
    this->dump_nodes(buf);
    this->dump_edges(buf);
    buf += "}\n";
    this->generate_graph(buf, fname);
}

static std::string join_components(std::string a, std::string b)
{
    if (a == "") return b;
    if (b == "") return a;
    return a + "." + b;
}

static std::string replace_trailing_digits(std::string str)
{
    size_t og_length = str.length();
    while (!str.empty() && std::isdigit(str.back()))
        str.pop_back();
    if (str.length() != og_length)
        str += "*";
    return str;
}

static std::vector<std::string> break_components(std::string path)
{
    std::vector<std::string> components;
    std::stringstream stream(path);
    std::string comp;
    while(std::getline(stream, comp, '.')) {
        components.push_back(comp);
    }
    return components;
}

static std::string colapse_path(std::string path, int depth)
{
    auto comps = break_components(path);
    int nr = std::min(comps.size(), (size_t)depth);
    std::string new_path = "";
    for (auto it = comps.rbegin(); it != comps.rend(); ++it) {
        std::string comp = it < comps.rbegin() + nr ? replace_trailing_digits(*it) : *it;
        new_path = join_components(comp, new_path); 
    }
    return new_path;
}

void gs::DumpGraphSingleton::colapse_numbered_nodes(void)
{
    std::map<std::string, std::string> new_graph;
    for (auto &edge : graph) {
        new_graph[colapse_path(edge.first, COLAPSE_NUMBERED_NODES_DEPTH)] =
            colapse_path(edge.second, COLAPSE_NUMBERED_NODES_DEPTH);
    }
    graph = new_graph;
}

void gs::DumpGraphSingleton::dump_node(std::string &buf, std::string full_path)
{
    std::string path = "";
    int depth = 0;
    for (auto comp : break_components(full_path)) {
        if (path != "")
            path += ".";
        path += comp;
        if (component_ids.find(path) == component_ids.end()) {
            component_ids[path] = nr_ids++;
            buf += "subgraph cluster" + std::to_string(component_ids[path])
                +  " {\n"
                +  "label=\"" + comp + "\";\ncluster"
                +   std::to_string(component_ids[path])
                +  "[shape=\"none\"][style=\"invis\"][label=\"\"];\n";
        } else {
            buf += "subgraph cluster"
                +  std::to_string(component_ids[path])
                +  " {\n";
        }
        depth++;
    }
    for ( ; depth ; depth--)
        buf += "}\n";
}

void gs::DumpGraphSingleton::dump_nodes(std::string &buf)
{
    for (auto const &edge : graph) {
        this->dump_node(buf, edge.first);
        this->dump_node(buf, edge.second);
    }
}

void gs::DumpGraphSingleton::dump_edges(std::string &buf)
{
    for (auto const &edge : graph) {
        int from_id = component_ids[edge.first];
        std::string from_node = "cluster" + std::to_string(from_id);

        int to_id = component_ids[edge.second];
        std::string to_node   = "cluster" + std::to_string(to_id);

        buf += from_node + " -> " + to_node
            +  "[ltail=\"" + from_node + "\"]"
            +  "[lhead=\"" + to_node + "\"];\n";
    }
}

static bool ends_with(std::string str, std::string suffix)
{
    return str.length() >= suffix.length() &&
           str.substr(str.length() - suffix.length(), suffix.length()) == suffix;
}

void gs::DumpGraphSingleton::generate_graph(std::string graph_str,
                                            std::string outfile)
{
    if (ends_with(outfile, ".txt")) {
        std::ofstream out;
        out.open("out.txt");
        out << graph_str;
        out.close();
    } else if (ends_with(outfile, ".svg")) {
        GVC_t *gvc = gvContext();
        agseterr(AGMAX);
        Agraph_t *g = agmemread(graph_str.c_str());
        gvLayout(gvc, g, "dot");
        gvRenderFilename(gvc, g, "svg", outfile.c_str());
        agclose(g);
        gvFreeLayout(gvc, g);
        gvFreeContext(gvc);
    } else {
        std::cout << "Unknown file extension for graph generation '" << outfile << "'\n";
        exit(1);
    }
    std::cout << "Graph generated at '" << outfile << "'\n";
}

gs::DumpGraphSingleton* gs::DumpGraphSingleton::pDumpGraphSingleton = NULL;

gs::DumpGraphSingleton* gs::DumpGraphSingleton::GetInstance()
{
    if (pDumpGraphSingleton == NULL) {
        pDumpGraphSingleton = new gs::DumpGraphSingleton();
    }
    return pDumpGraphSingleton;
}
