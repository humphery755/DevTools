package org.daemon.wrapper;

import java.util.concurrent.atomic.AtomicInteger;
import java.lang.reflect.Method;
/*
 * javac -cp . org/daemon/wrapper/LibC.java
 * javah -cp . org.daemon.wrapper.LibC
 * g++ -fPIC -c org_daemon_wrapper_LibC.cpp
 * g++ -shared -Wl,-soname,liborg_daemon_wrapper_LibC.so.1  -o liborg_daemon_wrapper_LibC.so  org_daemon_wrapper_LibC.o
 * java -cp . -Djava.library.path='.' org.daemon.wrapper.LibC
 * 
 *  1       HUP (hang up)
     2       INT (interrupt)
     3       QUIT (quit)
     6       ABRT (abort)
     6       ABRT (abort)
     9       KILL (non-catchable, non-ignorable kill)
     14      ALRM (alarm clock)
     15      TERM (software termination signal)
 */
public class LibC {
	static {
		try {
			System.loadLibrary("org_daemon_wrapper_LibC");
		} catch (Throwable e) {
			e.printStackTrace();
		}
	}

	public static native int init(int shmid);

	public static native int getpid();

	public static native int getwpid();// work process pid

	public static native int notifydp();// notify daemon process

	public static native int daemon(int nochdir, int noclose);

	public static native int fork();

	public static native void exit(int state);

	public static native int kill(int pid, int sigkill);

	public static native int[] pipe();

	public static native byte[] read(int fid, int bufLen);

	public static native int write(int fid, byte[] data);

	public static native int waitpid(int pid, int[] status, int options);

	static boolean runing;
	static Object obj = new Object();

	public static void main(String[] args) throws Exception {
		final int pid = getpid();
		System.out.println("LibC-" + pid + " *** main entry main, shmid: " + args[0]);
		try {
			int shmid = Integer.valueOf(args[0]);
			LibC.init(shmid);
			runing = true;
			String classze = args[1];
			String[] _args = null;
			if (args.length > 2) {
				_args = new String[args.length - 2];
				for (int i = 2; i < args.length; i++) {
					_args[i - 2] = args[i];
				}
			}else{
				_args=new String[]{};
			}
			Class entryCls = Class.forName(classze);
			Object entryObj = entryCls.newInstance();
			final Service s = entryObj instanceof Service ? (Service)entryObj:null;
			if(s!=null){
				s.init(_args);
				s.startup();
			}else{
				Method m1 = entryCls.getMethod("main",String[].class);
				m1.setAccessible(true);
				m1.invoke(entryCls,(Object)_args);
			}
			
			Runtime.getRuntime().addShutdownHook(new Thread(new Runnable() {
				private volatile boolean hasShutdown = false;
				private AtomicInteger shutdownTimes = new AtomicInteger(0);

				public void run() {
					synchronized (obj) {
						runing = false;
						System.out.println("LibC-" + pid + " *** main shutdown hook was invoked, " + this.shutdownTimes.incrementAndGet());
						if (!this.hasShutdown) {
							this.hasShutdown = true;
							long begineTime = System.currentTimeMillis();
							if(s!=null){
								try {
									s.destory();
								} catch (Throwable e) {
									e.printStackTrace();
								}
							}
							obj.notify();
							long consumingTimeTotal = System.currentTimeMillis() - begineTime;
							System.out.println("LibC-" + pid + " *** main shutdown hook over, consuming time total(ms): " + consumingTimeTotal);
						}
					}
				}
			}, "ShutdownHook"));
			synchronized (obj) {
				while (runing) {
					try {
						obj.wait();
					} catch (Throwable e) {
						e.printStackTrace();
					}
				}
			}
		} catch (Throwable e) {
			e.printStackTrace();
		}
	}

	/*
	 * if (LibC.daemon(1, 1) == -1) { System.exit(0); } System.out.println(
	 * "end daemon");
	 * 
	 * int file_pipes[] = LibC.pipe(); if (file_pipes == null) {
	 * System.exit(-1); } System.out.println("end pipe");
	 * 
	 * final int spid = LibC.fork(); if (spid == -1) { System.exit(-1); }
	 * 
	 * if (spid == 0) { LibC.main("com/alibaba/rocketmq/broker/daemon/Test",
	 * args); System.out.println("end LibC.main"); System.exit(0); }
	 * System.out.println("self pid: "+LibC.getpid()+", sub pid: " + spid);
	 */

	public interface Service {
		void init(String[] args);

		void startup();

		void destory();
	}
}
