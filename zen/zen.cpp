#include <zen/zen.h>
#include <zen/ConditionObject.h>
#include <zen/GlobalState.h>
#include <cassert>

namespace zen {

ZENAPI Exception::Exception(std::string const &msg) noexcept
    : msg(msg) {
}

ZENAPI Exception::~Exception() noexcept = default;

ZENAPI char const *Exception::what() const noexcept {
    return msg.c_str();
}

template <class T>
T *safe_at(std::map<std::string, std::unique_ptr<T>> const &m,
           std::string const &key, std::string const &msg) {
  auto it = m.find(key);
  if (it == m.end()) {
    throw Exception("invalid " + msg + " name: " + key);
  }
  return it->second.get();
}

template <class T>
T const &safe_at(std::map<std::string, T> const &m, std::string const &key,
          std::string const &msg) {
  auto it = m.find(key);
  if (it == m.end()) {
    throw Exception("invalid " + msg + " name: " + key);
  }
  return it->second;
}

template <class T, class S>
T const &safe_at(std::map<S, T> const &m, S const &key, std::string const &msg) {
  auto it = m.find(key);
  if (it == m.end()) {
    throw Exception("invalid " + msg + " as index");
  }
  return it->second;
}


#ifndef ZEN_FREE_IOBJECT
ZENAPI IObject::IObject() = default;
ZENAPI IObject::~IObject() = default;

ZENAPI std::shared_ptr<IObject> IObject::clone() const {
    return nullptr;
}

ZENAPI void IObject::visualize() {
}
#endif

ZENAPI INode::INode() = default;
ZENAPI INode::~INode() = default;

ZENAPI Context::Context() = default;
ZENAPI Context::~Context() = default;

ZENAPI Context::Context(Context const &other)
    : visited(other.visited)
{
}

ZENAPI Graph::Graph() = default;
ZENAPI Graph::~Graph() = default;

ZENAPI void INode::doComplete() {
    set_output("DST", std::make_shared<ConditionObject>());
    complete();
}

ZENAPI void INode::complete() {}

ZENAPI bool INode::checkApplyCondition() {
    if (has_input("COND")) {  // TODO: deprecate COND
        auto cond = get_input<zen::ConditionObject>("COND");
        if (!cond->get())
            return false;
    }

    if (has_option("ONCE")) {
        if (zen::state.frameid != 0)
            return false;
    }

    if (has_option("MUTE")) {
        auto desc = nodeClass->desc.get();
        set_output(desc->outputs[0], get_input(desc->inputs[0]));
        return false;
    }

    return true;
}

ZENAPI void INode::doApply() {
    for (auto const &[ds, bound]: inputBounds) {
        auto [sn, ss] = bound;
        graph->applyNode(sn);
        auto ref = graph->getNodeOutput(sn, ss);
        inputs[ds] = ref;
    }

    if (checkApplyCondition()) {
        apply();
    }

    if (has_option("VIEW")) {
        auto desc = nodeClass->desc.get();
        auto id = desc->outputs[0];
        auto obj = safe_at(outputs, id, "output");
        obj->visualize();
    }
}

ZENAPI bool INode::has_option(std::string const &id) const {
    return options.find(id) != options.end();
}

ZENAPI bool INode::has_input(std::string const &id) const {
    return inputs.find(id) != inputs.end();
}

ZENAPI std::shared_ptr<IObject> INode::get_input(std::string const &id) const {
    return safe_at(inputs, id, "input");
}

ZENAPI IValue INode::get_param(std::string const &id) const {
    return safe_at(params, id, "param");
}

ZENAPI void INode::set_output(std::string const &id, std::shared_ptr<IObject> &&obj) {
    outputs[id] = std::move(obj);
}

ZENAPI std::shared_ptr<IObject> const &Graph::getNodeOutput(
    std::string const &sn, std::string const &ss) const {
    auto node = safe_at(nodes, sn, "node");
    return safe_at(node->outputs, ss, "output");
}

ZENAPI void Graph::clearNodes() {
    nodes.clear();
}

ZENAPI void Graph::addNode(std::string const &cls, std::string const &id) {
    if (nodes.find(id) != nodes.end())
        return;  // no add twice, to prevent output object invalid
    auto cl = safe_at(sess->nodeClasses, cls, "node class");
    auto node = cl->new_instance();
    node->graph = this;
    node->myname = id;
    node->nodeClass = cl;
    nodes[id] = std::move(node);
}

ZENAPI void Graph::completeNode(std::string const &id) {
    safe_at(nodes, id, "node")->doComplete();
}

ZENAPI void Graph::applyNode(std::string const &id) {
    if (ctx->visited.find(id) != ctx->visited.end()) {
        return;
    }
    ctx->visited.insert(id);
    safe_at(nodes, id, "node")->doApply();
}

ZENAPI void Graph::applyNodes(std::vector<std::string> const &ids) {
    ctx = std::make_unique<Context>();
    for (auto const &id: ids) {
        applyNode(id);
    }
    ctx = nullptr;
}

ZENAPI void Graph::bindNodeInput(std::string const &dn, std::string const &ds,
        std::string const &sn, std::string const &ss) {
    safe_at(nodes, dn, "node")->inputBounds[ds] = std::pair(sn, ss);
}

ZENAPI void Graph::setNodeParam(std::string const &id, std::string const &par,
        IValue const &val) {
    safe_at(nodes, id, "node")->params[par] = val;
}

ZENAPI void Graph::setNodeOptions(std::string const &id,
        std::set<std::string> const &opts) {
    safe_at(nodes, id, "node")->options = opts;
}


ZENAPI Session::Session() {
    switchGraph("main");
}

ZENAPI Session::~Session() = default;

ZENAPI void Session::_defNodeClass(std::string const &id, std::unique_ptr<INodeClass> &&cls) {
    nodeClasses[id] = std::move(cls);
}

ZENAPI void Session::switchGraph(std::string const &name) {
    if (graphs.find(name) == graphs.end()) {
        auto subg = std::make_unique<zen::Graph>();
        subg->sess = this;
        graphs[name] = std::move(subg);
    }
    currGraph = graphs.at(name).get();
}

ZENAPI Graph &Session::getGraph() const {
    return *currGraph;
}

ZENAPI std::string Session::dumpDescriptors() const {
  std::string res = "";
  for (auto const &[key, cls] : nodeClasses) {
    res += "DESC:" + key + ":" + cls->desc->serialize() + "\n";
  }
  return res;
}


ZENAPI Session &getSession() {
    static std::unique_ptr<Session> ptr;
    if (!ptr) {
        ptr = std::make_unique<Session>();
    }
    return *ptr;
}



ParamDescriptor::ParamDescriptor(std::string const &type,
	  std::string const &name, std::string const &defl)
      : type(type), name(name), defl(defl) {}
ParamDescriptor::~ParamDescriptor() = default;

ZENAPI Descriptor::Descriptor() = default;
ZENAPI Descriptor::Descriptor(
  std::vector<std::string> const &inputs,
  std::vector<std::string> const &outputs,
  std::vector<ParamDescriptor> const &params,
  std::vector<std::string> const &categories)
  : inputs(inputs), outputs(outputs), params(params), categories(categories) {
    this->inputs.push_back("SRC");
    this->inputs.push_back("COND");  // TODO: deprecate COND
    this->outputs.push_back("DST");
}

ZENAPI std::string Descriptor::serialize() const {
  std::string res = "";
  res += "(" + join_str(inputs, ",") + ")";
  res += "(" + join_str(outputs, ",") + ")";
  std::vector<std::string> paramStrs;
  for (auto const &[type, name, defl] : params) {
      paramStrs.push_back(type + ":" + name + ":" + defl);
  }
  res += "(" + join_str(paramStrs, ",") + ")";
  res += "(" + join_str(categories, ",") + ")";
  return res;
}


} // namespace zen
