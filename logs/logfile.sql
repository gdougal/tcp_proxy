-- QUERY SIZE: 16
SELECT version()
-- QUERY SIZE: 276

SELECT
    db.oid as did, db.datname, db.datallowconn,
    pg_encoding_to_char(db.encoding) AS serverencoding,
    has_database_privilege(db.oid, 'CREATE') as cancreate, datlastsysoid,
    datistemplate
FROM
    pg_catalog.pg_database db
WHERE db.datname = current_database()
-- QUERY SIZE: 16
SELECT version()
-- QUERY SIZE: 276

SELECT
    db.oid as did, db.datname, db.datallowconn,
    pg_encoding_to_char(db.encoding) AS serverencoding,
    has_database_privilege(db.oid, 'CREATE') as cancreate, datlastsysoid,
    datistemplate
FROM
    pg_catalog.pg_database db
WHERE db.datname = current_database()
-- QUERY SIZE: 17
select version();
-- QUERY SIZE: 112
SELECT oid, pg_catalog.format_type(oid, NULL) AS typname FROM pg_catalog.pg_type WHERE oid IN (25) ORDER BY oid;
-- QUERY SIZE: 16
SELECT version()
-- QUERY SIZE: 276

SELECT
    db.oid as did, db.datname, db.datallowconn,
    pg_encoding_to_char(db.encoding) AS serverencoding,
    has_database_privilege(db.oid, 'CREATE') as cancreate, datlastsysoid,
    datistemplate
FROM
    pg_catalog.pg_database db
WHERE db.datname = current_database()
