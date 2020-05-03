# Operating System



#### 1. CFS
- Linux scheduler인 CFS를 응용 프로그램의 형태로 구현한다.
- parent process(ku_cfs.c)가 child process(ku_app.c)의 스케줄링을 수행한다.
- 이때 child process의 nice value를 (-2, -1, 0, 1, 2)로 제한한다.
- 명령행 인자는 nice value가 작은 순서부터 각 nice value의 child process 개수이다.
- (단 child process의 개수는 26개를 초과하지 않는다)
![architecture](https://user-images.githubusercontent.com/46714683/80897023-928e4c80-8d2f-11ea-857b-07608a360519.png)
