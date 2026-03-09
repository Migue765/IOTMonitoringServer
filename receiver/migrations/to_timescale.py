from django.db import migrations

# Migraciones para configuración de la base de datos Postgres con la extensión de Timescale.
# Si TimescaleDB no está instalado, estas operaciones se omiten (no-op) para permitir
# usar PostgreSQL estándar (p. ej. en laboratorios donde la instalación de TimescaleDB falló).


class Migration(migrations.Migration):

    dependencies = [
        ("receiver", "0001_initial"),
    ]

    operations = [
        # Crea la hipertabla de timescale con chunks de 3 días (solo si existe la extensión).
        migrations.RunSQL(
            sql="""
            DO $$
            BEGIN
              IF EXISTS (SELECT 1 FROM pg_extension WHERE extname = 'timescaledb') THEN
                PERFORM create_hypertable('"receiver_data"', 'time', chunk_time_interval => 259200000000);
              END IF;
            END $$;
            """,
            reverse_sql=migrations.RunSQL.noop,
        ),
        # Configura la compresión (solo si TimescaleDB está presente).
        migrations.RunSQL(
            sql="""
            DO $$
            BEGIN
              IF EXISTS (SELECT 1 FROM pg_extension WHERE extname = 'timescaledb') THEN
                ALTER TABLE "receiver_data"
                  SET (timescaledb.compress, timescaledb.compress_segmentby = 'station_id, measurement_id, base_time');
              END IF;
            END $$;
            """,
            reverse_sql=migrations.RunSQL.noop,
        ),
        # Comprime los datos cada 7 días (solo si TimescaleDB está presente).
        migrations.RunSQL(
            sql="""
            DO $$
            BEGIN
              IF EXISTS (SELECT 1 FROM pg_extension WHERE extname = 'timescaledb') THEN
                PERFORM add_compression_policy('"receiver_data"', 604800000000);
              END IF;
            END $$;
            """,
            reverse_sql=migrations.RunSQL.noop,
        ),
    ]
