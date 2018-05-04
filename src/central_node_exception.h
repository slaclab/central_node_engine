#ifndef CENTRAL_NODE_EXCEPTION_H
#define CENTRAL_NODE_EXCEPTION_H

/**
 * DbException class
 *
 * It is thrown by any Db* classes when there is a problem loading or
 * configuring the database.
 */
class CentralNodeException: public std::exception {
 public:
  explicit CentralNodeException(const char* message) : msg(message) {}
  explicit CentralNodeException(const std::string& message) : msg(message) {}
  virtual ~CentralNodeException() throw () {}
  virtual const char* what() const throw () {
    return msg.c_str();
  }

 protected:
  // Error message
  std::string msg;
};

#endif
