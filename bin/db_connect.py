import mysql.connector

class DBConnector:
    def __init__(self, host="dursley.socs.uoguelph.ca"):
        self.host = host
        self.connection = None

    def connect(self, username, password_, db_name):
        try:
            self.connection = mysql.connector.connect(
                host=self.host,
                user=username,
                password=password_,
                database=db_name
            )
            self.create_tables()  
        except mysql.connector.Error as err:
            self.connection = None
            return f"Database connection error: {err}"
        
        return 0

    def create_tables(self):
        queries = [
            """CREATE TABLE IF NOT EXISTS FILE (
                file_id INT AUTO_INCREMENT PRIMARY KEY,
                file_name VARCHAR(60) NOT NULL UNIQUE,
                last_modified DATETIME,
                creation_time DATETIME NOT NULL
            );""",
            """CREATE TABLE IF NOT EXISTS CONTACT (
                contact_id INT AUTO_INCREMENT PRIMARY KEY,
                name VARCHAR(256) NOT NULL,
                birthday DATETIME NULL,  
                anniversary DATETIME NULL, 
                file_id INT NOT NULL,
                FOREIGN KEY (file_id) REFERENCES FILE(file_id) ON DELETE CASCADE
            );"""
        ]

        cursor = self.connection.cursor()
        for query in queries:
            cursor.execute(query)
        self.connection.commit()
        cursor.close()

    def execute_query(self, query, values=None, fetch=False):
        if self.connection is None:
            return None

        cursor = self.connection.cursor()
        cursor.execute(query, values if values else ())

        result = None
        if fetch:
            result = cursor.fetchall()

        self.connection.commit()
        cursor.close()
        return result

    
    def close(self):
        if self.connection:
            self.connection.close()
