package org.daemon.test;

import org.daemon.wrapper.LibC;

public class MainTest{
	
	public static void main(String[] args) throws Exception {
		StringBuilder sb = new StringBuilder();
		if(args!=null){
			for(String s:args){
				if(sb.length()>0)sb.append(",");
				sb.append(s);
			}
		}        
		System.out.println("MainTest.main("+sb.toString()+")");
	}
}
