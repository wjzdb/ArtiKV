#include "art.hpp"
#include <cstdint>
#include <optional>
#include <span>

using namespace art;

Node* findChild(Node* node, unsigned char byte) {
    switch (node->type) {
    case NodeType::Node4: {
        auto n = static_cast<Node4 *>(node);
        return n->findChild(byte);
    }
    case NodeType::Node16: {
        auto n = static_cast<Node16 *>(node);
        return n->findChild(byte);
    }
    case NodeType::Node48: {
        auto n = static_cast<Node48 *>(node);
        return n->findChild(byte);
    }
    case NodeType::Node256: {
        auto n = static_cast<Node256 *>(node);
        return n->findChild(byte);
    }
    default:{
        throw "unknown node type";
    }
    }
}

void addChild(Node* node, unsigned char byte, Node* child) {
    switch (node->type) {
    case NodeType::Node4: {
        auto n = static_cast<Node4 *>(node);
        n->addChild(byte, child);
    }
    case NodeType::Node16: {
        auto n = static_cast<Node16 *>(node);
        n->addChild(byte, child);
    }
    case NodeType::Node48: {
        auto n = static_cast<Node48 *>(node);
        n->addChild(byte, child);
    }
    case NodeType::Node256: {
        auto n = static_cast<Node256 *>(node);
        n->addChild(byte, child);
    }
    default:{
        throw "unknown node type";
    }
    }
}

bool isLeaf(Node* node) {
    return NodeType::Leaf == node->type;
}