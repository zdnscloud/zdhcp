#include <fcntl.h>
#include <memory>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>   
#include <fstream>
#include <kea/logging/exception.h>
#include <kea/logging/fileoutputstream.h>

namespace kea{
namespace logging{

std::unique_ptr<FileOutputStream> 
FileOutputStream::openFile( const std::string& file_name, int flags, int permissions) {
  int fd = -1;
  auto fp = file_name.c_str();

  flags |= O_WRONLY;

  if ((flags & O_CREAT) == O_CREAT) {
    fd = open(fp, flags, permissions);
  } else {
    fd = open(fp, flags);
  }

  if (fd < 1) {
    RAISE_ERRNO(kIOError, "error opening file '%s'", fp);
  }
  return std::unique_ptr<FileOutputStream>(new FileOutputStream(fd, file_name, true));
}

FileOutputStream::FileOutputStream( int fd, std::string file_name, bool close_on_destroy) : 
    fd_(fd), file_name_(file_name), max_file_count_(DefaultMaxFileCount), 
    max_file_size_(DefaultMaxFileSize), close_on_destroy_(close_on_destroy) {
        struct stat info;  
        stat(file_name.c_str(), &info);  
        current_file_size_ = info.st_size;
}

FileOutputStream::~FileOutputStream() {
  if (fd_ >= 0 && close_on_destroy_) {
    close(fd_);
  }
}

size_t 
FileOutputStream::write(const std::string& data) {
    return write(data.c_str(), data.size());
}

size_t 
FileOutputStream::write(const char* data, size_t size) {
  int bytes_written = -1;

  if (max_file_size_ > 0 && current_file_size_.load() > max_file_size_) {
      if (fileRotate() == false) {
          return bytes_written;
      }
  }

  bytes_written = ::write(fd_, data, size);

  if (bytes_written < 0) {
    RAISE_ERRNO(kIOError, "write() failed");
  }

  current_file_size_ += bytes_written;

  return bytes_written;
}

bool
FileOutputStream::fileRotate() {
    close(fd_);
    fd_ = -1;
    if (max_file_count_ > 2) {
        for (int i=max_file_count_ - 2; i>0; --i) {
            char fname[file_name_.length() + 5];
            sprintf(fname, "%s.%03d", file_name_.c_str(), i);
            if (file_exists(std::string(fname))) {
                char old_fname[file_name_.length() + 5];
                sprintf(old_fname, "%s.%03d", file_name_.c_str(), i+1);
                if (rename(fname, old_fname)) {
                    return false;
                }
            }
        }
    }

    if (max_file_count_ > 1) {
        if (file_exists(file_name_)) {
            if (rename(file_name_.c_str(), (file_name_+".001").c_str())) {
                return false;
            }
        }
    }

    fd_ = open(file_name_.c_str(), O_CREAT | O_APPEND | O_WRONLY, 0666);

    if (fd_ < 1) {
        RAISE_ERRNO(kIOError, "error opening file '%s'", file_name_.c_str());
    }

    current_file_size_ = 0;

    return true;
}

bool 
FileOutputStream::file_exists(const std::string& file_name) {
    std::ifstream fs(file_name);
    return fs.is_open();
}

}
}
