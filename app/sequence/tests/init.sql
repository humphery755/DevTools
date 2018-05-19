CREATE TABLE `t_sequence` (
  `name` varchar(50) NOT NULL,
  `value` bigint(20) NOT NULL,
  `gmt_modified` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `step` int(11) NOT NULL DEFAULT '10000',
  `algorithm` tinyint(4) DEFAULT '0' COMMENT '0:默认递增;1:32位递增;2:64位递增;2:Snowflake',
  PRIMARY KEY (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;


DELIMITER // 
CREATE FUNCTION `seq_nextval_v2`(seq_name VARCHAR(50)) RETURNS VARCHAR(64) CHARSET latin1
BEGIN
    DECLARE lock_name VARCHAR(64);
    DECLARE retval VARCHAR(64);
    DECLARE val BIGINT;
    DECLARE inc INT;
    DECLARE seq_lock INT;
    SET lock_name = CONCAT(':fun:seq:',seq_name);
    SET val = 0;
    SET inc = 0;
    SET seq_lock = -1;
    SELECT GET_LOCK(lock_name, 15) INTO seq_lock;
    IF seq_lock = 1 THEN
      SELECT VALUE + step, step INTO val, inc FROM T_SEQUENCE WHERE NAME = seq_name FOR UPDATE;
      IF val > 0 THEN
          UPDATE T_SEQUENCE SET VALUE = val WHERE NAME = seq_name;
      END IF;
      SELECT RELEASE_LOCK(lock_name) INTO seq_lock;
      SELECT CONCAT(CAST((val - inc) AS CHAR),",",CAST(inc AS CHAR)) INTO retval;
    ELSE
      SELECT "-1,0" INTO retval;
    END IF;    
    RETURN retval;
END