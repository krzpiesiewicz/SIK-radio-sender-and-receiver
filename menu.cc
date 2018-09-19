#include <iostream>

#include "console_codes.h"
#include "menu.h"
#include "telnet.h"

#define HIGHLIGHT(to_highlight) cons::YELOW_BACKGROUND << cons::CYAN <<\
    to_highlight << cons::NO_COLOR

using namespace std;

void MenuElement::add_child(std::shared_ptr<MenuElement> const &child) {
  auto empty = shared_ptr<MenuElement>(nullptr);
  children.push_back(empty);
  (*children.rbegin()) = child;
  child->parent = this;
}

void MenuElement::exec_comm() {
  if (comm != nullptr) {
    comm();
  }
}

ostream &operator<<(ostream &os, MenuElement const &e) {
  os << e.title;
  return os;
}

ostream &operator<<(ostream &os, UIMenu const &ui_menu) {
  for (auto it = ui_menu.current->children.begin();
       it != ui_menu.current->children.end(); it++) {

    os << "  ";
    if (it != ui_menu.selected) {
      os << "  " << *(*it);
    } else {
      os << "> " << *(*it);
    }
    os << endl << telnet::NAOFFD;
  }
  return os;
}

void UIMenu::set_root(std::shared_ptr<MenuElement> root_) {
  root = root_;
  current = root.get();
  selected = root->children.begin();
}

void UIMenu::select_next() {
  auto it = selected;
  it++;
  if (it != current->children.end()) {
    selected = it;
  }
}

void UIMenu::select_prev() {
  if (selected != current->children.begin()) {
    selected--;
  }
}

void UIMenu::come_back() {
  if (current->parent != nullptr) {
    current = current->parent;
    selected = current->children.begin();
  }
}

void UIMenu::go_further_or_exec() {
  if ((*selected)->children.empty()) {
    (*selected)->exec_comm();
  } else {
    current = selected->get();
    selected = current->children.begin();
  }
}