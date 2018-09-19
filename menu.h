#ifndef MENU_H
#define MENU_H

#include <list>
#include <string>
#include <memory>
#include <istream>
#include <functional>

class UIMenu;

class MenuElement {
public:
  std::string title;
  std::list<std::shared_ptr<MenuElement>> children;
  MenuElement *parent;
  std::function<void(void)> comm;

  friend class UIMenu;

  friend std::ostream &operator<<(std::ostream &os, MenuElement const &e);

  friend std::ostream &operator<<(std::ostream &os, UIMenu const &ui_menu);

public:
  MenuElement(std::string title_ = {},
              std::function<void(void)> comm_ = nullptr) :
    title(title_), parent(nullptr), comm(comm_) {}

  void add_child(std::shared_ptr<MenuElement> const &child);

  void attach_command(std::function<void(void)> comm_) {comm = comm_;}

  void exec_comm();

  std::string get_title() {return title;};
};

std::ostream &operator<<(std::ostream &os, MenuElement const &e);

class UIMenu {
public:
  std::shared_ptr<MenuElement> root;
  MenuElement *current;
  std::list<std::shared_ptr<MenuElement>>::iterator selected;

  friend std::ostream &operator<<(std::ostream &os, UIMenu const &ui_menu);

public:
  void set_root(std::shared_ptr<MenuElement> root_);

  MenuElement *getSelected() {return selected->get();}

  void select_next();

  void select_prev();

  void come_back();

  void go_further_or_exec();
};

std::ostream &operator<<(std::ostream &os, UIMenu const &ui_menu);

#endif // MENU_H
