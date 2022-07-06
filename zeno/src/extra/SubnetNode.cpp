#include <zeno/core/INode.h>
#include <zeno/core/Session.h>
#include <zeno/core/Graph.h>
#include <zeno/extra/SubnetNode.h>
#include <zeno/types/DummyObject.h>
#include <zeno/utils/log.h>

namespace zeno {

ZENO_API SubnetNode::SubnetNode() : subgraph(std::make_unique<Graph>())
{}

ZENO_API SubnetNode::~SubnetNode() = default;

ZENO_API void SubnetNode::apply() {
    for (auto const &[key, nodeid]: subgraph->subInputNodes) {
        zeno::log_warn("input {} {}", key, nodeid);
        auto node = safe_at(subgraph->nodes, nodeid, "node name").get();
        if (has_input(key)) {
            node->inputs["_IN_port"] = get_input(key);
            node->inputs["_IN_hasValue"] = objectFromLiterial(true);
        } else {
            node->inputs["_IN_port"] = std::make_shared<DummyObject>();
            node->inputs["_IN_hasValue"] = objectFromLiterial(false);
        }
    }

    std::set<std::string> ids;
    for (auto const &[key, nodeid]: subgraph->subOutputNodes) {
        ids.insert(nodeid);
    }
    subgraph->applyNodes(ids);

    for (auto const &[key, nodeid]: subgraph->subOutputNodes) {
        zeno::log_warn("output {} {}", key, nodeid);
        auto node = safe_at(subgraph->nodes, nodeid, "node name").get();
        auto it = node->outputs.find("_OUT_port");
        if (it != node->outputs.end()) {
            set_output(key, it->second);
        }
    }
}

}
