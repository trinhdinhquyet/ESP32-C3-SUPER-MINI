CREATE TABLE HLIG_ADO.dbo.dwm_raw_data (
	[_Identify] int IDENTITY(1,1) NOT NULL,
	[_Locked] bit NULL,
	[_SortKey] numeric(28,14) NULL,
	write_time datetime NULL,
	zigbee_data text COLLATE Chinese_PRC_CI_AS NULL,
	run_time float NULL,
	down_time float NULL,
	machine_code nvarchar(16) COLLATE Chinese_PRC_CI_AS NULL,
	remark nvarchar(128) COLLATE Chinese_PRC_CI_AS NULL,
	write_date date NULL,
	zigbee_hash varchar(40) COLLATE SQL_Latin1_General_CP1_CI_AS NULL,
	CONSTRAINT PrimaryKey_OEE_RAW_DATA PRIMARY KEY ([_Identify])
);
 CREATE NONCLUSTERED INDEX IX_dwm_raw_data_zigbee_hash ON HLIG_ADO.dbo.dwm_raw_data (  zigbee_hash ASC  )  
	 WITH (  PAD_INDEX = OFF ,FILLFACTOR = 100  ,SORT_IN_TEMPDB = OFF , IGNORE_DUP_KEY = OFF , STATISTICS_NORECOMPUTE = OFF , ONLINE = OFF , ALLOW_ROW_LOCKS = ON , ALLOW_PAGE_LOCKS = ON  )
	 ON [PRIMARY ] ;

CREATE TABLE HLIG_ADO.dbo.dwm_maching_list (
	[_Identify] int NOT NULL,
	[_Locked] bit NULL,
	[_SortKey] numeric(28,14) NULL,
	machine_code nvarchar(32) COLLATE SQL_Latin1_General_CP1_CI_AS NULL,
	machine_id nvarchar(32) COLLATE SQL_Latin1_General_CP1_CI_AS NULL,
	machine_name nvarchar(32) COLLATE SQL_Latin1_General_CP1_CI_AS NULL,
	machine_model nvarchar(64) COLLATE SQL_Latin1_General_CP1_CI_AS NULL,
	dept_id nvarchar(16) COLLATE SQL_Latin1_General_CP1_CI_AS NULL,
	dept_name nvarchar(32) COLLATE SQL_Latin1_General_CP1_CI_AS NULL,
	remark nvarchar(128) COLLATE SQL_Latin1_General_CP1_CI_AS NULL,
	line_id nvarchar(16) COLLATE SQL_Latin1_General_CP1_CI_AS NULL,
	line_name nvarchar(32) COLLATE SQL_Latin1_General_CP1_CI_AS NULL
);
-- Step 1: Add zigbee_hash column (SHA-1 = 40 hex chars)
IF NOT EXISTS (
    SELECT 1 FROM INFORMATION_SCHEMA.COLUMNS 
    WHERE TABLE_SCHEMA = 'dbo' 
    AND TABLE_NAME = 'dwm_raw_data' 
    AND COLUMN_NAME = 'zigbee_hash'
)
BEGIN
    ALTER TABLE dbo.dwm_raw_data ADD zigbee_hash VARCHAR(40) NULL;
    PRINT 'Column zigbee_hash added successfully.';
END
ELSE
BEGIN
    PRINT 'Column zigbee_hash already exists. Skipping.';
END
GO

-- Step 2: Create index on zigbee_hash for O(1) duplicate checking
IF NOT EXISTS (
    SELECT 1 FROM sys.indexes 
    WHERE name = 'IX_dwm_raw_data_zigbee_hash' 
    AND object_id = OBJECT_ID('dbo.dwm_raw_data')
)
BEGIN
    CREATE INDEX IX_dwm_raw_data_zigbee_hash 
    ON dbo.dwm_raw_data (zigbee_hash);
    PRINT 'Index IX_dwm_raw_data_zigbee_hash created successfully.';
END
ELSE
BEGIN
    PRINT 'Index IX_dwm_raw_data_zigbee_hash already exists. Skipping.';
END
GO

-- Step 3 (Optional): Backfill hash for existing records
-- Uncomment and run if you want to hash existing zigbee_data
-- WARNING: This may take a while on large tables
/*
UPDATE dbo.dwm_raw_data 
SET zigbee_hash = CONVERT(VARCHAR(40), HASHBYTES('SHA1', CAST(zigbee_data AS NVARCHAR(MAX))), 2)
WHERE zigbee_hash IS NULL 
AND zigbee_data IS NOT NULL;
PRINT 'Backfill completed.';
GO
*/
