yum -y remove postgresql
cd postgresql
rpm -ivh postgresql95-devel-9.5.4-1PGDG.rhel6.x86_64.rpm openssl-1.0.1e-48.el6_8.1.x86_64.rpm openssl-devel-1.0.1e-48.el6_8.1.x86_64.rpm postgresql95-libs-9.5.4-1PGDG.rhel6.x86_64.rpm postgresql95-server-9.5.4-1PGDG.rhel6.x86_64.rpm postgresql95-9.5.4-1PGDG.rhel6.x86_64.rpm postgresql95-contrib-9.5.4-1PGDG.rhel6.x86_64.rpm --force
cd ..
service postgresql-9.5 initdb
chkconfig postgresql-9.5 on
\cp postgresql.conf /var/lib/pgsql/9.5/data/postgresql.conf
\cp pg_hba.conf /var/lib/pgsql/9.5/data/pg_hba.conf
service postgresql-9.5 restart
sudo -u postgres psql postgres -c "CREATE DATABASE zdns;"
sudo -u postgres psql postgres -c "CREATE USER zdns WITH PASSWORD 'zdns';"
sudo -u postgres psql postgres -c "GRANT ALL PRIVILEGES ON DATABASE zdns TO zdns;"

