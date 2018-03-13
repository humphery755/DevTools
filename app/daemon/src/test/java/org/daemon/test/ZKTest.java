package org.daemon.test;

import java.util.Collections;
import java.util.List;

import org.I0Itec.zkclient.IZkChildListener;
import org.I0Itec.zkclient.ZkClient;
import org.apache.zookeeper.CreateMode;
import org.daemon.wrapper.LibC;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class ZKTest implements LibC.Service{
	private static final Logger log = LoggerFactory.getLogger(ZKTest.class);
	private ZkClient zkClient;
	private String lockPath="/org/test";
	private String tmpLockPath;
	private String zkServer="192.168.1.12:3181";
    private int connectionTimeout = 50000;
	
	public void init(String[] args){
		System.out.println("ZKTest.init entry main");
		zkClient = new ZkClient(zkServer, connectionTimeout);
		createRecursive(lockPath);
	}
	
	private void createRecursive(String fullPath) {
		if (zkClient.exists(fullPath))
			return;

		String[] pathParts = fullPath.replaceFirst("/", "").split("/");
		StringBuilder path = new StringBuilder();
		for (String pathElement : pathParts) {
			path.append("/").append(pathElement);
			String pathString = path.toString();
			if (!zkClient.exists(pathString)) {
				zkClient.createPersistent(pathString, null);
			}
		}
	}
	public void startup(){
		System.out.println("ZKTest.startup was invoked" );
		tmpLockPath = zkClient.create(lockPath+"/lock", System.currentTimeMillis(), CreateMode.EPHEMERAL_SEQUENTIAL);

		zkClient.subscribeChildChanges(lockPath, new IZkChildListener() {

			public void handleChildChange(String parentPath, List<String> currentChilds) throws Exception {
				log.warn("the node {} child be changed ===>{}", parentPath, currentChilds);
				if (currentChilds == null || currentChilds.size() == 0) {
					log.info("<" + parentPath + "> is deleted");
					return;
				}
				Collections.sort(currentChilds);
				String selfPath = tmpLockPath.substring(lockPath.length() + 1);
				int index = currentChilds.indexOf(selfPath);
				if (index == 0) {
					// inddx == 0, 说明thisNode在列表中最小, 当前client获得锁
					masterProcess();
				}else if(index > 0){
					slaveProcess();
				}
			}
		});

		List<String> children = zkClient.getChildren(lockPath);
		if (children.size() > 0) {
			Collections.sort(children);
			String selfPath = tmpLockPath.substring(lockPath.length() + 1);
			int index = children.indexOf(selfPath);
			if (index == 0) {
				// inddx == 0, 说明thisNode在列表中最小, 当前client获得锁
				try {
					masterProcess();
				} catch (Exception e) {
					e.printStackTrace();
				}
			}else if(index > 0){
				slaveProcess();
			}
		}
	}
	
	private synchronized void masterProcess() {
		int pid = LibC.getwpid();
		System.out.println("ZKTest.masterProcess getwpid:"+pid);
		if(pid>0)
			LibC.kill(pid,15);
		else{
			LibC.notifydp();
		}
	}
	private synchronized void slaveProcess() {
		int pid = LibC.getwpid();
		System.out.println("ZKTest.slaveProcess getwpid:"+pid);
		if(pid>0)
		LibC.kill(pid,15);
		else{
			LibC.notifydp();
		}
	}
	
	public void destory(){
		System.out.println("ZKTest.destory was invoked" );
		zkClient.close();
	}
}
