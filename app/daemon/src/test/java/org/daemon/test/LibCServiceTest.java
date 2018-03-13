package org.daemon.test;

import org.daemon.wrapper.LibC;

public class LibCServiceTest  implements LibC.Service{
	static LibCServiceTest self=new LibCServiceTest();
	
	public static void main(String[] args) throws Exception {
		System.out.println("LibCServiceTest.main entry main");
		self.init(args);
		self.startup();
	}
	
	public void init(String[] args) {
		System.out.println("LibCServiceTest.init");
	}
	public void destory() {
		System.out.println("LibCServiceTest.destory");
	}

	public void startup() {
		System.out.println("LibCServiceTest.startup()");
	}
}
