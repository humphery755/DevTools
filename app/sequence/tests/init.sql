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
    DECLARE retval VARCHAR(64);
    DECLARE val BIGINT;
    DECLARE inc INT;
    DECLARE i INT;
    DECLARE rowcount INT;
    SET val = 0;
    SET inc = 0;
    SET i = 0;
    SET rowcount = 0;
    SET retval = "-1,0";
    outer_label:
    WHILE(i<100) DO
        SELECT VALUE + step, step INTO val, inc FROM t_sequence WHERE NAME = seq_name;
        SELECT FOUND_ROWS() INTO rowcount;
        IF rowcount > 0 THEN
            UPDATE t_sequence SET VALUE = val WHERE NAME = seq_name AND VALUE=val-inc;
            SELECT ROW_COUNT() INTO rowcount;
            IF rowcount > 0 THEN
              SELECT CONCAT(CAST((val - inc) AS CHAR),",",CAST(inc AS CHAR)) INTO retval;
              LEAVE  outer_label;
            END IF;
        END IF;
        SET i=i+1;
    END WHILE;
    RETURN retval;
END;
