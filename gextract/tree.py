# tree.py
# copy from https://nlg0.isi.edu/projects/hiero/browser/branches/mira
# David Chiang <chiang@isi.edu>

# Copyright (c) 2004-2006 University of Maryland. All rights
# reserved. Do not redistribute without permission from the
# author. Not for commercial use.

import re

class Node:
    "Tree node.  length is # of nodes.  self.parent.children[self.order]=self"
    def __init__(self, label, children):
        self.label = label
        self.children = children
        self.length = 0
        for i in range(len(self.children)):
            self.children[i].parent = self
            self.children[i].order = i
            self.length += self.children[i].length
        if len(self.children) == 0:
            self.length = 1
        self.parent = None
        self.order = 0

    def __getitem__(self,gornaddr):
        "gornaddr is a list [] for root [0] for first child of root, [0,2] for 3rd child of that, etc."
        if len(gornaddr) == 0:
            return self
        return self.children[gornaddr[0]][gornaddr[1:]]

    def filter_inplace(self):
        "top down, remove subtree with root node t unless keep(t).  returns new tree (destructive update of old one)"
        if not keep(self):
            return None
        return filter_inplace_children(keep)

    def filter_inplace_children(self,keep):
        "like filter_inplace, but always keep root"
        newc=None
        oldc=self.children
        newi=0
        for i in range(0,len(oldc)):
            c=oldc[i]
            kc=c.filter(keep)
            if kc is None:
                self.length -= 1
                if newc is None:
                    newc=oldc[0:i]
                    newi=i
            else:
                if newc is not None:
                    newc.append(c)
                    c.order=newi
                newi+=1
        if newc is not None:
            self.children=newc
        return self

    def filter(self,keep):
        "like filter_inplace, but doesn't destroy original"
        if not keep(self):
            return None
        return self.filter_children(keep)

    def filter_children(self,keep):
        "like filter, but always keep root"
        return Node(self.label,filter(lambda x:x is not None,(c.filter(keep) for c in self.children)))

    def zip_postorder(self,other):
        "other and self have the same shape; yield tuples (selfnode,othernode) in postorder"
        for c,oc in zip(self.children,other.children):
            for p in c.zip_postorder(oc):
                yield p
        yield (self,other)

    def set_attr_from(self,srct,dest_attr,src_attr='label',default=None,skip_missing=False):
        "setattr(node,dest_attr,getattr(srctnode,src_attr)) for every node in self, and every srctnode in srct.  if a srct node is missing the attribute, use default, or don't set the attribute at all if skip_missing"
        if hasattr(srct,src_attr):
            setattr(self,dest_attr,getattr(srct,src_attr))
        elif not skip_missing:
            setattr(self,dest_attr,default)
        for c,sc in zip(self.children,srct.children):
            c.set_attr_from(sc,dest_attr,src_attr,default,skip_missing)

    def find_ancestor(self,pred):
        "return closest ancestor such that pred(ancestor), or else None"
        p=self.parent
        while p is not None and not pred(p):
            p=p.parent
        return p

    def find_descendants(self,pred,r):
        "return list of frontier of descendants such that pred(descendant).  frontier not meaning just leaves - internal nodes with pred(node) stop recursion"
        if r is None:
            r=[]
            for c in self.children: c.find_descendants(pred,r)
            return r
        else:
            if pred(self):
                r.append(self)
            else:
                for c in self.children: c.find_descendants(pred,r)

    def __copy__(self):
        return self.relabel(lambda x:x.label)
        #return Node(self.label,[c.__copy__() for c in self.children])

    def relabel(self,newlabel):
        "copy w/ t'.label = newlabel(t)"
        return Node(newlabel(self),[c.relabel(newlabel) for c in self.children])

    def yield_labels(self):
        return [t.label for t in self.frontier()]

    def __str__(self):
        if len(self.children) != 0:
            s = "(" + str(self.label)
            for child in self.children:
                s += " " + child.__str__()
            s += ")"
            return s
        else:
            s = str(self.label)
            s = re.sub("\(", "-LRB-", s)
            s = re.sub("\)", "-RRB-", s)
            return s

    def descendant(self, addr):
        if len(addr) == 0:
            return self
        else:
            return self.children[addr[0]].descendant(addr[1:])

    def insert_child(self, i, child):
        child.parent = self
        self.children[i:i] = [child]
        for j in range(i,len(self.children)):
            self.children[j].order = j
        if len(self.children) > 1:
            self.length += child.length
        else:
            self.length = child.length # because self.label changes into nonterminal

    def delete_child(self, i):
        self.children[i].parent = None
        self.children[i].order = 0
        self.length -= self.children[i].length
        if i != -1:
            self.children[i:i+1] = []
        else:
            self.children[-1:] = []
        for j in range(i,len(self.children)):
            self.children[j].order = j

    def detach(self):
        self.parent.delete_child(self.order)

    def delete_clean(self):
        "Cleans up childless ancestors"
        parent = self.parent
        self.detach()
        if len(parent.children) == 0:
            parent.delete_clean()

    def frontier(self):
        if len(self.children) != 0:
            l = []
            for child in self.children:
                l.extend(child.frontier())
            return l
        else:
            return [self]

    def is_dominated_by(self, node):
        return self is node or (self.parent != None and self.parent.is_dominated_by(node))

    def dominates(self, arg):
        if type(arg) is list:
            flag = 1
            for node in arg:
                flag = flag and node.is_dominated_by(self)
            return flag
        elif isinstance(arg, Node):
            return node.is_dominated_by(self)

    def nodes_byspan(self):
        """return a hash which maps from spans to lists of nodes (top-down)."""
        s = {}

        def visit(node, i):
            s.setdefault((i,i+node.length), []).append(node)
            for child in node.children:
                visit(child, i)
                i += child.length

        visit(self, 0)

        return s

    def spans_bynode(self):
        """return a hash which maps from nodes to spans"""
        spans = {}
        nj = 0
        for node in self.postorder():
            if node.is_terminal():
                spans[node] = (nj,nj+1)
                nj += 1
            else:
                spans[node] = (spans[node.children[0]][0], spans[node.children[-1]][1])
        return spans

    def preorder(self):
        yield self
        for child in self.children:
            for desc in child.preorder():
                yield desc

    def postorder(self):
        for child in self.children:
            for desc in child.postorder():
                yield desc
        yield self

    def is_terminal(self):
        return len(self.children) == 0

    def is_preterminal(self):
        return len(self.children) == 1 and self.children[0].is_terminal()

def scan_tree(tokens, pos):
    try:
        if tokens[pos] == "(":
            if tokens[pos+1] == "(":
                label = ""
                pos += 1
            else:
                label = tokens[pos+1]
                pos += 2
            children = []
            (child, pos) = scan_tree(tokens, pos)
            while child != None:
                children.append(child)
                (child, pos) = scan_tree(tokens, pos)
            if tokens[pos] == ")":
                return (Node(label, children), pos+1)
            else:
                return (None, pos)
        elif tokens[pos] == ")":
            return (None, pos)
        else:
            label = tokens[pos]
            label = label.replace("-LRB-", "(")
            label = label.replace("-RRB-", ")")
            return (Node(label,[]), pos+1)
    except IndexError:
        return (None, pos)

tokenizer = re.compile(r"\(|\)|[^()\s]+")

def str_to_tree(s):
    (tree, n) = scan_tree(tokenizer.findall(s), 0)
    return tree

