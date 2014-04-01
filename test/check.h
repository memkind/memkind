#ifndef check_include_h
#define check_include_h

class Check {
  public:
    Check(const void *ptr, size_t size);
    ~Check();
    int check_node_hbw(size_t num_bandwidth, const int *bandwidth);
    int check_page_size(size_t page_size);
  private:
    void **address;
    int pagemap_fd;
    int num_address;
    int is_correct_page_size(void *vaddr, size_t page_size);
};

#endif
