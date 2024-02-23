#include <mutex>

// class Storage
// {
// private:
//     /* data */
// public:
//   Storage(/* args */);
//   ~Storage();
//   Storage(Storage const&) = delete;
//   void operator=(Storage const&)  = delete;
// };

extern std::mutex preferences_lock;
