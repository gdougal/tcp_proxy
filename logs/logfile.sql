-- QUERY SIZE: 173
SET DateStyle=ISO; SET client_min_messages=notice; SELECT set_config('bytea_output','hex',false) FROM pg_settings WHERE name = 'bytea_output'; SET client_encoding='UNICODE';
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
-- QUERY SIZE: 766

        SELECT
            roles.oid as id, roles.rolname as name,
            roles.rolsuper as is_superuser,
            CASE WHEN roles.rolsuper THEN true ELSE roles.rolcreaterole END as
            can_create_role,
            CASE WHEN roles.rolsuper THEN true
            ELSE roles.rolcreatedb END as can_create_db,
            CASE WHEN 'pg_signal_backend'=ANY(ARRAY(
                SELECT pg_catalog.pg_roles.rolname FROM
                pg_catalog.pg_auth_members m JOIN pg_catalog.pg_roles ON
                (m.roleid = pg_catalog.pg_roles.oid) WHERE
                 m.member = roles.oid)) THEN True
            ELSE False END as can_signal_backend
        FROM
            pg_catalog.pg_roles as roles
        WHERE
            rolname = current_user
-- QUERY SIZE: 17
select version();
-- QUERY SIZE: 112
SELECT oid, pg_catalog.format_type(oid, NULL) AS typname FROM pg_catalog.pg_type WHERE oid IN (25) ORDER BY oid;
