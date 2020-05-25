# Operating System



## 1. CFS
- Linux scheduler인 CFS를 응용 프로그램의 형태로 구현한다.
- parent process(ku_cfs.c)가 child processes(ku_app.c)의 스케줄링을 수행한다.
- child process의 nice value 범위 : (-2, -1, 0, 1, 2)
- 명령행 인자: nice value가 작은 순서부터 각 nice value의 child process 개수
- (단 child process의 개수는 26개를 초과하지 않는다)
![architecture](https://user-images.githubusercontent.com/46714683/80897023-928e4c80-8d2f-11ea-857b-07608a360519.png)
 
 
 
 
 
 
  
## 2. MMU
- Memory Management Unit 구현
- Multi-Level Page Table 구조로 physical memeory 관리 (8-bit addressing)
- Swap out policy: FIFO
- Swap out PF: Page에 대응되는 Page Frame
- Usage: ku_cpu (pid_va_list.txt) (pmem_size) (swap_size)
![슬라이드1](https://user-images.githubusercontent.com/46714683/82776010-6b392400-9e84-11ea-8142-cd5225adfc16.JPG)
![슬라이드2](https://user-images.githubusercontent.com/46714683/82776013-6e341480-9e84-11ea-8ada-4996a2155227.JPG)
