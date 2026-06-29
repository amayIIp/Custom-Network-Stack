#pragma once
#include <string>
#include <cstdint>
#include <cstddef>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace itch {

class MemoryMappedFile {
private:
    const uint8_t* m_data = nullptr;
    size_t m_size = 0;
#if defined(_WIN32)
    HANDLE m_file_handle = INVALID_HANDLE_VALUE;
    HANDLE m_mapping_handle = NULL;
#else
    int m_fd = -1;
#endif

public:
    MemoryMappedFile() = default;
    ~MemoryMappedFile() { close(); }

    // Disable copy
    MemoryMappedFile(const MemoryMappedFile&) = delete;
    MemoryMappedFile& operator=(const MemoryMappedFile&) = delete;

    bool open(const std::string& filepath) {
        close();
#if defined(_WIN32)
        m_file_handle = CreateFileA(
            filepath.c_str(), 
            GENERIC_READ, 
            FILE_SHARE_READ, 
            NULL, 
            OPEN_EXISTING, 
            FILE_ATTRIBUTE_NORMAL, 
            NULL
        );
        if (m_file_handle == INVALID_HANDLE_VALUE) {
            return false;
        }

        LARGE_INTEGER size;
        if (!GetFileSizeEx(m_file_handle, &size)) {
            close();
            return false;
        }
        m_size = static_cast<size_t>(size.QuadPart);

        if (m_size == 0) {
            return true; 
        }

        m_mapping_handle = CreateFileMappingA(
            m_file_handle, 
            NULL, 
            PAGE_READONLY, 
            0, 
            0, 
            NULL
        );
        if (m_mapping_handle == NULL) {
            close();
            return false;
        }

        m_data = reinterpret_cast<const uint8_t*>(
            MapViewOfFile(m_mapping_handle, FILE_MAP_READ, 0, 0, 0)
        );
        if (m_data == nullptr) {
            close();
            return false;
        }
#else
        m_fd = ::open(filepath.c_str(), O_RDONLY);
        if (m_fd < 0) {
            return false;
        }

        struct stat st;
        if (fstat(m_fd, &st) < 0) {
            close();
            return false;
        }
        m_size = st.st_size;

        if (m_size == 0) {
            return true;
        }

        void* mapped = mmap(nullptr, m_size, PROT_READ, MAP_PRIVATE, m_fd, 0);
        if (mapped == MAP_FAILED) {
            close();
            return false;
        }
        m_data = reinterpret_cast<const uint8_t*>(mapped);
#endif
        return true;
    }

    void close() {
        if (m_data) {
#if defined(_WIN32)
            UnmapViewOfFile(m_data);
#else
            munmap(const_cast<uint8_t*>(m_data), m_size);
#endif
            m_data = nullptr;
        }
#if defined(_WIN32)
        if (m_mapping_handle != NULL) {
            CloseHandle(m_mapping_handle);
            m_mapping_handle = NULL;
        }
        if (m_file_handle != INVALID_HANDLE_VALUE) {
            CloseHandle(m_file_handle);
            m_file_handle = INVALID_HANDLE_VALUE;
        }
#else
        if (m_fd >= 0) {
            ::close(m_fd);
            m_fd = -1;
        }
#endif
        m_size = 0;
    }

    const uint8_t* data() const { return m_data; }
    size_t size() const { return m_size; }
};

} // namespace itch
