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
