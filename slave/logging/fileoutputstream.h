#include <fcntl.h>
#include <memory>
#include <mutex>
#include <atomic>

namespace kea{
namespace logging{

class FileOutputStream {
public:

  static std::unique_ptr<FileOutputStream> openFile(
      const std::string& file_name,
      int flags = O_CREAT | O_APPEND,
      int permissions = 0666);

  explicit FileOutputStream(int fd, std::string file_name, bool close_on_destroy = false);

  ~FileOutputStream();

  size_t write(const std::string& data);

  size_t write(const char* data, size_t size);

  mutable std::mutex mutex;

  static const size_t DefaultMaxFileCount   = 10;
  static const size_t DefaultMaxFileSize = 100 * 1000 * 1000; //100m byte

private:
  bool fileRotate();
  bool file_exists(const std::string& file_name);

  int fd_;
  std::string file_name_;
  size_t max_file_count_;
  size_t max_file_size_;
  std::atomic<size_t> current_file_size_;
  bool close_on_destroy_;

};

}
}
