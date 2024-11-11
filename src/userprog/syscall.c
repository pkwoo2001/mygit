#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static void syscall_handler (struct intr_frame *);
void syscall_init (void);
void halt (void);
void exit (int status);
pid_t exec (const char *cmd_lime);
int wait (pid_t pid);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);
bool check_addr_vaild (const void *addr, int size);
static void get_arguments (uint32_t *esp, int num, uint32_t *arg);
//static bool check_arg_addr(const void *arg);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&fs_lock);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  if (!check_addr_vaild(f->esp,4)) 
    exit(-1); 
  char* arg[3];
  //printf("syscall : %d\n", *(uint32_t *)(f->esp));
  //hex_dump(f->esp,f->esp, 150, true); 
  switch (*(uint32_t *)(f->esp)) {
    case SYS_HALT:
      halt();
      break;
    case SYS_EXIT:
      if(!check_addr_vaild(f->esp+1,4))
      {
        exit(-1);
      }
      get_arguments(f->esp,1,arg);
      exit(arg[0]);
      //exit(*(uint32_t *)(f->esp + 20));
      break;
    case SYS_EXEC:
      if(!check_addr_vaild(f->esp+1,4))
      {
        exit(-1);
      }
      get_arguments(f->esp, 1, arg);
      f->eax = exec((const char *)arg[0]); 
      break;
    case SYS_WAIT:
      if(!check_addr_vaild(f->esp+1,4))
      {
        exit(-1);
      }
      get_arguments(f->esp,1, arg);
      f->eax = wait(arg[0]); 
      break;
    case SYS_CREATE:
      if(!check_addr_vaild(f->esp+1,4) || !check_addr_vaild(f->esp+2,4))
      {
        exit(-1);
      }
      get_arguments(f->esp,2, arg); 
      f->eax = create((const char *)arg[0], arg[1]);
      break;
    case SYS_REMOVE:
      if(!check_addr_vaild(f->esp+1,4))
      {
        exit(-1);
      }
      get_arguments(f->esp,1, arg); 
      f->eax = remove(arg[0]);
      break;
    case SYS_OPEN:
      if(!check_addr_vaild(f->esp+1,4))
      {
        exit(-1);
      }
      get_arguments(f->esp,1, arg);
      f->eax = open((const char*)arg[0]);
      break;
    case SYS_FILESIZE:
      if(!check_addr_vaild(f->esp+1,4))
      {
        exit(-1);
      }
      get_arguments(f->esp,1, arg);
      f->eax = filesize(arg[0]);
      break;
    case SYS_READ:
      if(!check_addr_vaild(f->esp+1,4) || !check_addr_vaild(f->esp+2,4) || !check_addr_vaild(f->esp+3,4))
      {
        exit(-1);
      }
      get_arguments(f->esp,3, arg);
      if(!check_addr_vaild(arg[1],4))// 버퍼 주소 valid 확인 bad ptr case
      {
        exit(-1);
      }
      f->eax = read(arg[0],(void *)arg[1],arg[2]);
      break;
    case SYS_WRITE:
      if(!check_addr_vaild(f->esp+1,4) || !check_addr_vaild(f->esp+2,4) || !check_addr_vaild(f->esp+3,4))
      {
        exit(-1);
      }
      get_arguments(f->esp, 3, arg);
      if(!check_addr_vaild(arg[1],4)) // 버퍼 주소 valid 확인 bad ptr case
      {
        exit(-1);
      }
      f->eax = write(arg[0], (const void *) arg[1], arg[2]);
      //f->eax = write((int)*(uint32_t *)(f->esp+20), (void *)*(uint32_t *)(f->esp + 24), (unsigned)*((uint32_t *)(f->esp + 28)));
      break;
    case SYS_SEEK:
      if(!check_addr_vaild(f->esp+1,4) || !check_addr_vaild(f->esp+2,4))
      {
        exit(-1);
      }
      get_arguments(f->esp,2, arg);
      seek(arg[0],arg[1]);
      break;
    case SYS_TELL:
      if(!check_addr_vaild(f->esp+1,4))
      {
        exit(-1);
      }
      get_arguments(f->esp,1, arg);
      f->eax = tell(arg[0]);
      break;
    case SYS_CLOSE:
      if(!check_addr_vaild(f->esp+1,4))
      {
        exit(-1);
      }
      get_arguments(f->esp,1, arg);
      close(arg[0]) ; //change
      break;
  }
  //printf("system call!\n");
  //thread_exit ();
}
void halt (void) {
  shutdown_power_off();
}

void exit (int status)
{ 
 struct thread *cur = thread_current();
 cur->exit_status = status;

 printf("%s: exit(%d)\n", cur->name, status);
 for (int i = 3; i < 128; i++) { //added
      if (thread_current()->fd[i] != NULL) {
          close(i);
      }   
  }   
 thread_exit();
}

pid_t exec (const char *cmd_line) {
  return process_execute(cmd_line);
}

int wait (pid_t pid) {
  return process_wait(pid);
}
bool create (const char *file, unsigned initial_size){ //fslock check
  if(file == NULL){
    exit(-1);
  }
  //printf("if enter?");
  lock_acquire(&fs_lock);
  bool result = filesys_create(file,initial_size);
  lock_release(&fs_lock);
  //printf("if enter?");
  return result;
}
bool remove (const char *file){ //fslock check
  if(file == NULL){
    exit(-1);
  }
  lock_acquire(&fs_lock);
  bool result = filesys_remove(file);
  lock_release(&fs_lock);
  return result;
}
int open (const char *file){ //fslock check
  int i;
  struct file* fp;
  if(file == NULL){
    exit(-1);
  }
  lock_acquire(&fs_lock);
  fp = filesys_open(file);
  lock_release(&fs_lock); 
  if (fp == NULL) {
      return -1; 
  } else {
    for (i = 3; i < 128; i++) {
      if (thread_current()->fd[i] == NULL) {
        thread_current()->fd[i] = fp;
        return i;
      }   
    }   
  }
  return -1; 
}

int filesize (int fd){ //fslock check
  int file_size;
  if (thread_current()->fd[fd] == NULL) {
      exit(-1);
  }
  lock_acquire(&fs_lock);
  file_size = file_length(thread_current()->fd[fd]);
  lock_release(&fs_lock);
  return file_size;
}
void seek (int fd, unsigned position){ //fslock check
  if (thread_current()->fd[fd] == NULL) {
      return;
  }
  lock_acquire(&fs_lock);
  file_seek(thread_current()->fd[fd], position);
  lock_release(&fs_lock);
}
unsigned tell (int fd){ //fslock check
  int position;
  if (thread_current()->fd[fd] == NULL) {
    return 0;
  }
  lock_acquire(&fs_lock);
  position = file_tell(thread_current()->fd[fd]);
  lock_release(&fs_lock);
  return position;
}
void close (int fd){ //fslock check
  struct file* fp;
  if (thread_current()->fd[fd] == NULL) {
      exit(-1);
  }
  fp = thread_current()->fd[fd];
  thread_current()->fd[fd] = NULL;
  lock_acquire(&fs_lock);
  file_close(fp);
  lock_release(&fs_lock);
}

int read (int fd, void* buffer, unsigned size) { //fslock check
  int i;
  int read_size = 0;
  if (fd == 0) {
    for (i = 0; i < size; i ++) {
        *((char *) (buffer +i)) = input_getc(); 
        read_size++; 
      }
  } else if(fd >2){
    if (thread_current()->fd[fd] == NULL) {
      exit(-1);
    }
    else{
      lock_acquire(&fs_lock);
      read_size = file_read(thread_current()->fd[fd],buffer,size);
      lock_release(&fs_lock);
    }
  }
  return read_size;
}

int write (int fd, const void *buffer, unsigned size) { //fslock check
  int write_size = 0 ;
  if (fd == 1) {
    putbuf(buffer, size);
    return size;
  } else if (fd > 2) {
    if (thread_current()->fd[fd] == NULL) {
      exit(-1);
    }
    lock_acquire(&fs_lock);
    write_size = file_write(thread_current()->fd[fd], buffer, size);
    lock_release(&fs_lock);
    return write_size;
  } 
}
 static void
 get_arguments (uint32_t *esp, int num, uint32_t *arg)
 { 
  for(int i = 0; i < num; i++) 
      { 
        arg[i] = *((esp+1)+i);
      } 
}
bool check_addr_vaild (const void *addr, int size)
 { 
  int i; 
  for(i = 0; i < size; i++) 
    { 
      if(addr + i == NULL)
      {
        return false;
      }
      if(!is_user_vaddr(addr + i))
      {
        return false;
      }
      if(pagedir_get_page(thread_current()->pagedir, addr + i) == NULL)
      {
        return false;
      }
    } 
  return true; 
} 



