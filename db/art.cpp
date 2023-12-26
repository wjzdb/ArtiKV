#include "art.hpp"
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <optional>
#include <span>
#include <vector>

using namespace art;

static NodeRef NULL_NODE(nullptr);

static void copyInnerNodeProps(InnerNode* dst, const InnerNode* src) {
    dst->children_count.store(src->children_count);
    dst->prefix_len = src->prefix_len;
    std::copy(src->prefix.begin(), src->prefix.end(), dst->prefix.begin());
}

void ART::insert(ARTDataRef key, ARTData &&value) {
    ARTData key_data = std::vector<unsigned char>(key.begin(), key.end());
    bool updated = insertRecursively(this->root, key, makeLeafNode(std::move(key_data), std::move(value)), 0);
    if (!updated) {
        this->tree_size++;
    }
}

std::optional<ARTDataRef> ART::search(ARTDataRef key) {
    return searchRecursively(this->root, key, 0);
}

void ART::remove(ARTDataRef key) {
    
}

bool ART::insertRecursively(NodeRef& node, ARTDataRef key, LeafNode* leaf, int depth) {
    if (node == nullptr) {
        replace(node, leaf);
        return true;
    }
    // expand node.
    if (node.load()->type() == NodeType::Leaf) {
        LeafNode* current_leaf = dynamic_cast<LeafNode*>(node.load());
        // Key exists, updating existing value. 
        if (current_leaf->leafMatches(key, depth)) {
            replace(node, leaf);
            // delete original existing node.
            delete node.load();
            return true;
        }
        // Split the leaf into a Node4.
        Node4* new_node = makeNode4();
        ARTDataRef key2 = current_leaf->keyRef();
        int i = depth;
        for (i = depth; key[i] == key2[i]; i++) {
            new_node->prefix[i-depth] = key[i];
        }
        new_node->prefix_len = i - depth;
        new_node->addChild(key[depth], leaf);
        new_node->addChild(key2[depth], node);
        replace(node, new_node);
        return false;
    }
    // `node` is not a leaf, it should be an inner node.
    InnerNode* inner_node = dynamic_cast<InnerNode*>(node.load());
    int p = inner_node->commonPrefixLen(key, depth);
    // Prefix mismatch, we should adjust the compressed path.
    if (size_t(p) != inner_node->prefix_len) {
        Node4* new_node = makeNode4();
        new_node->addChild(key[depth+p], leaf);
        new_node->addChild(inner_node->prefix[depth+p], node);
        new_node->prefix_len = p;
        memcpy(new_node->prefix.data(), inner_node->prefix.data(), p);
        inner_node->prefix_len -= (p + 1);
        memmove(inner_node->prefix.data(), inner_node->prefix.data()+p+1, inner_node->prefix_len);
        replace(node, new_node);
        return false;
    }
    depth = depth + inner_node->prefix_len;
    NodeRef& next = inner_node->findChild(key[depth]);
    if (next != nullptr) {
        return insertRecursively(next, key, leaf, depth+1);
    } else {
        if (inner_node->isFull()) {
            InnerNode* new_inner_node = inner_node->grow();
            replace(node, new_inner_node);
            delete node.load();
            inner_node = new_inner_node;
        }
        inner_node->addChild(key[depth], leaf);
        return false;
    }
}

std::optional<ARTDataRef> ART::searchRecursively(Node* node, ARTDataRef key, int depth) {
    if (node == nullptr) {
        return std::nullopt;
    }
    if (node->type() == NodeType::Leaf) {
        LeafNode* leaf_node = dynamic_cast<LeafNode *>(node);
        if (leaf_node->leafMatches(key, depth)) {
            return leaf_node->valueRef();
        }
        return std::nullopt;
    }
    InnerNode* inner_node = dynamic_cast<InnerNode *>(node);
    depth += inner_node->commonPrefixLen(key, depth);
    Node *next = inner_node->findChild(key[depth]);
    return searchRecursively(next, key, depth);
}

size_t ART::size() {
    return this->tree_size.load();
}

int InnerNode::commonPrefixLen(ARTDataRef key, int depth) {
    int iterTime = std::min(std::min(MAX_PREFIX_LEN, this->prefix_len), key.size() - depth);
    int prefixLen;
    for (prefixLen = 0; prefixLen < iterTime; prefixLen++) {
        if (this->prefix[prefixLen] != key[depth + prefixLen]) {
            return prefixLen;
        }
    }
    return prefixLen;
}

NodeRef& Node4::findChild(unsigned char byte) {
    for (int i = 0; i < this->children_count; i++) {
        if (byte == this->keys[i]) {
            return this->children[i];
        }
    }
    // TODO: Is there a better way to represent we didn't find a node?
    return NULL_NODE;
}

void Node4::addChild(unsigned char byte, Node *child) {
    int idx;
    for (idx = 0; idx < this->children_count; idx++) {
        if (this->keys[idx] > byte) break;
    }
    
    // Make room for new child.
    memmove(this->keys.data()+idx+1, this->keys.data()+idx, this->children_count-idx);
    // TODO: try to optimize the performance of this for loop.
    for (int i = children_count-1; i > idx; i--) {
        children[i].store(children[i-1].load());
    }
    // Insert new node.
    this->keys[idx] = byte;
    this->children[idx].store(child);
    this->children_count++;
}

bool Node4::isFull() {
    return this->children_count >= 4;
}

InnerNode* Node4::grow() {
    Node16* new_node = makeNode16();
    std::copy(this->keys.begin(), this->keys.end(), new_node->keys.begin());
    size_t size = this->children.size();
    for (size_t i = 0; i < size; i++) {
        new_node->children[i].store(this->children[i].load());
    }
    copyInnerNodeProps(new_node, this);
    return new_node;
}

NodeRef& Node16::findChild(unsigned char byte) {
    // TODO: use SIMD to improve performance.
    auto target = std::lower_bound(this->keys.begin(), this->keys.end(), byte);
    if (target == this->keys.end() || *target != byte) {
        return NULL_NODE;
    }
    return this->children[std::distance(this->keys.begin(), target)];
}

void Node16::addChild(unsigned char byte, Node *child) {
    // TODO: use SIMD to improve performance.
    int idx;
    for(idx = 0; idx < this->children_count; idx++) {
        if (this->keys[idx] > byte) {
            break;
        }
    }
    memmove(this->keys.data()+idx+1,this->keys.data()+idx, this->children_count-idx);
    for (int i = children_count-1; i > idx; i--) {
        children[i].store(children[i-1].load());
    }
    this->keys[idx] = byte;
    this->children[idx].store(child);
    this->children_count++;
}

bool Node16::isFull() {
    return this->children_count >= 16;
}

InnerNode* Node16::grow() {
    Node48* new_node = makeNode48();
    for(int i = 0; i < this->children_count; i++) {
        new_node->children[i].store(this->children[i].load());
        new_node->keys[this->keys[i]] = i + 1;
    }
    copyInnerNodeProps(new_node, this);
    return new_node;
}

void Node48::addChild(unsigned char byte, Node *child) {
    int idx = 0;
    while (this->children[idx] != nullptr) {
        idx++;
    }
    this->children[idx] = child;
    this->keys[byte] = idx + 1;
    this->children_count++;
}

NodeRef& Node48::findChild(unsigned char byte) {
    auto idx = this->keys[byte];
    // Array 'keys' stores number 1~49, which indicates the child position
    // in array 'children'.
    if (idx > 0) {
        return this->children[idx-1];
    }
    return NULL_NODE;
}

bool Node48::isFull() {
    return this->children_count >= 48;
}

InnerNode* Node48::grow() {
    Node256 *new_node = makeNode256();
    for (int i = 0; i < 256; i++) {
        if (this->keys[i] > 0) {
            new_node->children[i].store(this->children[this->keys[i]-1].load());
        }
    }
    copyInnerNodeProps(new_node, this);
    return new_node;
}

void Node256::addChild(unsigned char byte, Node *child) {
    this->children_count++;
    this->children[byte] = child;
}

NodeRef& Node256::findChild(unsigned char byte) {
    return this->children[byte];
}

bool Node256::isFull() {
    return false;
}

InnerNode* Node256::grow() {
    return nullptr;
}

bool LeafNode::leafMatches(ARTDataRef key, int depth) {
    if (key.size() != this->key.size()) {
        return false;
    }
    return std::equal(this->key.begin()+depth, this->key.end(), key.begin());
}

void art::replace(NodeRef &node, Node *newNode) {
    node.store(newNode);
}

Node4* art::makeNode4() {
    // TODO: use Arena to alloc memory.
    return new Node4();
}

Node16* art::makeNode16() {
    // TODO: use Arena to alloc memory.
    return new Node16();
}

Node48* art::makeNode48() {
   // TODO: use Arena to alloc memory. 
   return new Node48();
}

Node256* art::makeNode256() {
    // TODO: use Arena to alloc memory.
    return new Node256();
}

LeafNode* art::makeLeafNode(ARTData&& key, ARTData&& value) {
    // TODO: use Arena to alloc memory.
    return new LeafNode(std::move(key), std::move(value));
}