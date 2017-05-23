package com.kedacom.ga;

import java.math.BigInteger;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Random;


import org.elasticsearch.transport.TransportSettings;
import org.elasticsearch.common.settings.Settings;
import org.elasticsearch.common.settings.Setting;
import org.elasticsearch.common.settings.Setting.Property;

public class MyAllocator {
	
	public static Setting<Settings> DATA_PATH_ATTR = Setting.groupSetting("path.data_attr.", Property.Final, Property.NodeScope);
	public static Setting<Integer> DATA_NODE_COUNT = Setting.intSetting("data_node.count", 1, 1, Property.Final, Property.NodeScope);
	public static Setting<Integer> DATA_NODE_INDEX = Setting.intSetting("node.data_node_index", 0, 0, Property.Final, Property.NodeScope);
	
	public static Setting<String> INDEX_ALLOCATION_TYPE = Setting.simpleString("index.allocation_type", Property.IndexScope);
	
	public static List<Integer >randomInts(int num_shards, long seed) {
		List<Integer> l = new ArrayList<Integer>(num_shards);
		for (int i = 0; i < num_shards; i++) {
			l.add(i);
		}
		
		Random rnd = new Random(seed);
		Collections.shuffle(l, rnd);
		return l;
	}
	
	public static List<Integer> getShardSlots(String index_name, int shard_id, int num_nodes) {
		try {
			MessageDigest digest = MessageDigest.getInstance("SHA-256");
			byte[] index_name_hash = digest.digest(index_name.getBytes());
			int hash_2 = Math.abs(new BigInteger(index_name_hash).hashCode());
			
			List<Integer> l_1 = randomInts(num_nodes, hash_2 + shard_id);
			List<Integer> l_2 = adjustList(l_1, (hash_2 + shard_id)%num_nodes);
			
			return l_2;
		} catch (NoSuchAlgorithmException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		return null;
	}

	public static List<Integer> getShardSlots(String index_name, int shard_id, int num_nodes, int num_replica) {
		List<Integer> l = getShardSlots(index_name, shard_id, num_nodes);
		int head = l.get(0);
		for (int i = 0; i < num_replica; i++) {
			l.remove(new Integer((head+i+1)%num_nodes));
			l.add(i+1, (head+i+1)%num_nodes);
		}
		return l;
	}

	private static List<Integer> adjustList(List<Integer> list, int offset) {
//		System.out.println(offset);
		for (int i = 0; i < list.size(); i++) {
			if (list.get(0) != offset) {
				list.add(list.remove(0));
			}
		}
		return list;
	}

	public static void main(String[] args) {
		// TODO Auto-generated method stub
		int num_shards = 14;
		
		out:
		for (int i = 0; i < 5; i++) {
			for (int j = 0; j < 5; j++) {
				if (j == 2) {
					continue out; 
				}
			}
		}
		
		for (int i = 0; i < num_shards; i++) {
			System.out.println(getShardSlots("ffff", i, 4, 1));
		}
		
//		List<Integer> a = randomInts(5, 23423);
//		System.out.println(a);
	}

}
