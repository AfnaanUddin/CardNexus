import ctypes
from asciimatics.screen import Screen
from asciimatics.widgets import Frame, Layout, ListBox, Button, Text, Divider, Label, PopUpDialog, TextBox
from asciimatics.scene import Scene
import os
import weakref
from db_connect import DBConnector
from datetime import datetime

# Load C shared library
lib = ctypes.CDLL(os.path.join(os.path.dirname(__file__), "libvcparser.so"))

# Define function signatures based on your wrapper functions

lib.get_vcard_name.argtypes = [ctypes.c_char_p]
lib.get_vcard_name.restype = ctypes.c_char_p  # Returns a string

lib.save_vcard.argtypes = [ctypes.c_char_p, ctypes.c_void_p]
lib.save_vcard.restype = ctypes.c_int  # Returns an error code

lib.update_vcard_name.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
lib.update_vcard_name.restype = ctypes.c_int  # Returns an error code

lib.get_vcard_details.argtypes = [ctypes.c_char_p]
lib.get_vcard_details.restype = ctypes.c_char_p  # Returns a string

def show_error(screen, message, buttons=None, on_close=None, theme='warning'):
    # if buttons is None:
    #     buttons = ["OK"]

    # # Create the dialog
    # dialog = PopUpDialog(screen, message, buttons, on_close=on_close, theme=theme)
    # # Create a new scene with the dialog and play it immediately
    # screen.play([Scene([dialog], -1)], stop_on_resize=True)

    if buttons is None:
        buttons = ["OK"]

    # Create the dialog first
    dialog = PopUpDialog(
        screen,
        message,
        buttons,
        on_close=on_close,
        has_shadow=True,
        theme=theme
    )

    # Use weak reference to avoid circular dependencies
    weak_dialog = weakref.ref(dialog)

    # Create a proper closure for dialog removal
    def _safe_close(selected_index):
        # Get current scene before any potential screen changes
        current_scene = screen.current_scene
        
        # Schedule dialog removal for next frame
        def _remove():
            nonlocal current_scene
            dialog_ref = weak_dialog()
            if dialog_ref and dialog_ref in current_scene.effects:
                current_scene.remove_effect(dialog_ref)
            if on_close:
                on_close(selected_index)
        
        # Queue the removal for safe execution
        screen.set_timer(0, _remove)

    # Assign the safe close handler
    dialog.on_close = _safe_close

    # Add to current scene
    screen.current_scene.add_effect(dialog)

db_connector = DBConnector()

vcard_dir = os.path.join(os.path.dirname(__file__), "cards")

def parse_vcard_date(date_str):
    try:
        # Convert 'YYYYMMDDTHHMMSS' → 'YYYY-MM-DD HH:MM:SS'
        return datetime.strptime(date_str, "%Y%m%dT%H%M%S").strftime("%Y-%m-%d %H:%M:%S")
    except ValueError:
        return None  # Return None for invalid dates

def scan_vcards():
    for filename in os.listdir(vcard_dir):
        if filename.endswith(".vcf"):
            filepath = os.path.join(vcard_dir, filename).encode()

            # Extract details from vCard (FN, Birthday, etc.)
            name, birthday, anniversary = get_vcard_details(filepath)

            birthday = parse_vcard_date(birthday) if birthday else None
            anniversary = parse_vcard_date(anniversary) if anniversary else None

            result = db_connector.execute_query("SELECT file_id FROM FILE WHERE file_name = %s", (filename,), fetch=True)
            if result:
                continue

            last_modified = datetime.fromtimestamp(os.path.getmtime(os.path.join(vcard_dir, filename)))
        
            # Insert into FILE table
            db_connector.execute_query(
                "INSERT INTO FILE (file_name, last_modified, creation_time) VALUES (%s, %s, NOW())",
                (filename, last_modified)
            )

            # Get file_id
            result = db_connector.execute_query(
                "SELECT file_id FROM FILE WHERE file_name = %s", (filename,), fetch=True
            )
            file_id = result[0][0] if result else None

            if file_id:
                # Insert into CONTACT table
                db_connector.execute_query(
                    "INSERT INTO CONTACT (name, birthday, anniversary, file_id) VALUES (%s, %s, %s, %s)",
                    (name, birthday, anniversary, file_id)
                )

def get_vcard_details(filepath):
    name, birthday, anniversary = None, None, None

    with open(filepath, "r") as file:
        for line in file:
            if line.startswith("FN:"):
                name = line.strip().split(":")[1]
            elif line.startswith("BDAY:"):
                birthday = line.strip().split(":")[1]
            elif line.startswith("ANNIVERSARY:"):
                anniversary = line.strip().split(":")[1]


    return name, birthday, anniversary

class LoginView(Frame):
    def __init__(self, screen):
        super(LoginView, self).__init__(screen, screen.height, screen.width, title="Database Login")

        # Define UI Layout
        layout = Layout([1], fill_frame=True)
        self.add_layout(layout)

        # Username Input
        self.username = Text("Username: ", name="username")
        layout.add_widget(self.username)

        # Password Input (Hidden)
        self.password = Text("Password: ", name="password", hide_char="*")
        layout.add_widget(self.password)

        # Database Name Input
        self.db_name = Text("Database Name: ", name="db_name")
        layout.add_widget(self.db_name)

        # Buttons
        layout.add_widget(Button("OK", self.attempt_login))
        layout.add_widget(Button("Cancel", self.exit_app))

        self.fix()  # Fix layout

    def attempt_login(self):
        username = self.username.value.strip()
        password = self.password.value.strip()
        db_name = self.db_name.value.strip()

        if not username or not password or not db_name:
            show_error(self.screen, "All fields are required!")
            return
        
        res = db_connector.connect(username=username,password_=password,db_name=db_name)

        if res != 0:
            show_error(self.screen, res)
            return 
        
        scan_vcards()

        self.screen.play([Scene([MainView(self.screen)], -1)])

    def exit_app(self):
        db_connector.close()
        raise SystemExit()

class MainView(Frame):
    def __init__(self, screen):
        super(MainView, self).__init__(screen, screen.height, screen.width, title="📂 vCard Manager")

        self.palette["background"] = (Screen.COLOUR_BLACK, Screen.A_NORMAL, Screen.COLOUR_BLACK)
        self.palette["title"] = (Screen.COLOUR_WHITE, Screen.A_BOLD, Screen.COLOUR_BLACK)
        self.palette["text"] = (Screen.COLOUR_WHITE, Screen.A_NORMAL, Screen.COLOUR_BLACK)
        self.palette["button"] = (Screen.COLOUR_CYAN, Screen.A_BOLD, Screen.COLOUR_BLACK)
        self.palette["highlight"] = (Screen.COLOUR_BLACK, Screen.A_BOLD, Screen.COLOUR_CYAN)
        
        # Layout for UI elements
        layout = Layout([1], fill_frame=True)
        self.add_layout(layout)

        valid_vcards = []
        
        for filename in os.listdir(vcard_dir):
            if filename.endswith(".vcf"):
                full_path = os.path.join(vcard_dir, filename).encode()
                if lib.validate_vcard(full_path) == 0:
                    valid_vcards.append((filename, filename))

        self.vcard_list = ListBox(height=10, options=valid_vcards, add_scroll_bar=True, on_select=self.handle_enter)

        layout.add_widget(Label("Available vCards:"))
        layout.add_widget(self.vcard_list)
        layout.add_widget(Divider())

        # Buttons
        layout.add_widget(Button("View/Edit vCard", self.view_card))
        layout.add_widget(Button("Create New vCard", self.create_card))
        layout.add_widget(Button("DB Queries", self.db_queries))
        layout.add_widget(Button("Exit", self.quit))

        self.fix()

    def view_card(self):
        if self.vcard_list.value:
            self.screen.play([Scene([VCardView(self.screen, self.vcard_list.value)], -1)])

    def create_card(self):
        self.screen.play([Scene([CreateCardView(self.screen)], -1)])

    def db_queries(self):
        self.screen.play([Scene([DBView(self.screen)], -1)])
    
    def handle_enter(self):
        if self.vcard_list.value:
            self.screen.play([Scene([VCardView(self.screen, self.vcard_list.value)], -1)])

    def quit(self):
        db_connector.close()
        raise SystemExit()

class VCardView(Frame):
    def __init__(self, screen, filename):
        super(VCardView, self).__init__(screen, screen.height, screen.width, title=f"Viewing: {filename}")
        
        self.filename = os.path.join(os.path.dirname(__file__), "bin", "cards", filename).encode()
        
        contact_name = lib.get_vcard_name(self.filename)
        details = lib.get_vcard_details(self.filename)

        self.name = contact_name.decode() if contact_name else "Unknown"
        details_str = details.decode() if details else "N/A"

        self.details_dict = self.parse_details(details_str)

        self.palette["background"] = (Screen.COLOUR_BLACK, Screen.A_NORMAL, Screen.COLOUR_BLACK)
        self.palette["title"] = (Screen.COLOUR_WHITE, Screen.A_BOLD, Screen.COLOUR_BLACK)
        self.palette["text"] = (Screen.COLOUR_WHITE, Screen.A_NORMAL, Screen.COLOUR_BLACK)
        self.palette["button"] = (Screen.COLOUR_CYAN, Screen.A_BOLD, Screen.COLOUR_BLACK)
        self.palette["highlight"] = (Screen.COLOUR_BLACK, Screen.A_BOLD, Screen.COLOUR_CYAN)
        self.palette["bold_text"] = (Screen.COLOUR_WHITE, Screen.A_NORMAL, Screen.COLOUR_BLACK)

        layout = Layout([1])
        self.add_layout(layout)
        
        file_path = self.details_dict.get("File", "Unknown")
        file_name = os.path.basename(file_path) if file_path != "Unknown" else "Unknown"
        layout.add_widget(Label(f"File: {file_name}"))
        
        layout_2 = Layout([1, 13])  
        self.add_layout(layout_2)

        self.name_label = Label("Contact:")
        layout_2.add_widget(self.name_label, 0) 
        
        self.name_edit = Text(name="name")
        self.name_edit.value = self.name
        self.name_edit.custom_colour = "bold_text"
        layout_2.add_widget(self.name_edit, 1)  

        layout_3 = Layout([1], fill_frame=True)
        self.add_layout(layout_3)

        birthday_str = self.details_dict.get("Birthday", "None")

        # Extract content inside brackets
        if "DateTime:" in birthday_str and "[" in birthday_str and "]" in birthday_str:
            formatted_birthday = birthday_str.split("[")[1].strip("]")  # Get content inside brackets
        else:
            formatted_birthday = birthday_str  # If no formatting is needed

        layout_3.add_widget(Label(f"Birthday: {formatted_birthday}"))

        anniversary_str = self.details_dict.get("Anniversary", "None")

        # Extract content inside brackets
        if "DateTime:" in anniversary_str and "[" in anniversary_str and "]" in anniversary_str:
            formatted_anniversary = anniversary_str.split("[")[1].strip("]")  # Get content inside brackets
        else:
            formatted_anniversary = anniversary_str  # If no formatting is needed

        layout_3.add_widget(Label(f"Anniversary: {formatted_anniversary}"))

        layout_3.add_widget(Label(f"Other Properties: {self.details_dict.get('Other Props', '0')}"))
        layout_3.add_widget(Divider())

        layout_3.add_widget(Button("OK", self.save_changes))
        layout_3.add_widget(Button("Cancel", self.go_back))

        self.fix()

    def parse_details(self, details_str):
        details_dict = {}
        for line in details_str.split("\n"):
            if ": " in line:  # Ensure valid key-value pair
                key, value = line.split(": ", 1)
                details_dict[key.strip()] = value.strip()
        return details_dict

    def save_changes(self):
        new_name = self.name_edit.value.strip()
        
        if not new_name:
            show_error(self.screen, "Contact name cannot be empty!")
            return
        
        result = lib.get_vcard_name(self.filename)

        old_name = result.decode() if result else "N/A"

        if old_name != new_name:        
            result = lib.update_vcard_name(self.filename, new_name.encode())

            if result != 0:  
                show_error(self.screen, "Failed to update name!")
                return
            
            db_connector.execute_query("UPDATE FILE SET last_modified = NOW() WHERE file_name = %s",(self.filename.decode().split('/')[-1],))
            # Get file_id
            result = db_connector.execute_query(
                "SELECT file_id FROM FILE WHERE file_name = %s", (self.filename.decode().split('/')[-1],), fetch=True
            )
            file_id = result[0][0] if result else None

            db_connector.execute_query("UPDATE CONTACT SET name = %s WHERE file_id = %s",(new_name, file_id))



        self.go_back()

    def go_back(self):
        self.screen.play([Scene([MainView(self.screen)], -1)])  

class CreateCardView(Frame):
    def __init__(self, screen):
        super(CreateCardView, self).__init__(screen, screen.height, screen.width, title="Create New vCard")

        self.palette["background"] = (Screen.COLOUR_BLACK, Screen.A_NORMAL, Screen.COLOUR_BLACK)
        self.palette["title"] = (Screen.COLOUR_WHITE, Screen.A_BOLD, Screen.COLOUR_BLACK)
        self.palette["text"] = (Screen.COLOUR_WHITE, Screen.A_NORMAL, Screen.COLOUR_BLACK)
        self.palette["button"] = (Screen.COLOUR_CYAN, Screen.A_BOLD, Screen.COLOUR_BLACK)
        self.palette["highlight"] = (Screen.COLOUR_BLACK, Screen.A_BOLD, Screen.COLOUR_CYAN)
        self.palette["bold_text"] = (Screen.COLOUR_WHITE, Screen.A_NORMAL, Screen.COLOUR_BLACK)

        layout = Layout([1], fill_frame=True)
        self.add_layout(layout)

        self.filename_input = Text(label="Filename (.vcf):", name="filename")
        self.filename_input.custom_colour = "bold_text"
        layout.add_widget(self.filename_input)
        self.contact_input = Text(label="Contact:", name="contact")
        self.contact_input.custom_colour = "bold_text"
        layout.add_widget(self.contact_input)
        layout.add_widget(Divider())

        layout.add_widget(Button("Save", self.save_card))
        layout.add_widget(Button("Cancel", self.go_back))

        self.fix()

    def save_card(self):

        filename = self.filename_input.value.strip()
        contact = self.contact_input.value.strip()

        if not filename or not contact:
            show_error(self.screen, "Any field cannot be empty.")
            return
        
        if not filename.endswith(".vcf"):
            show_error(self.screen, "Invalid file extension. Only .vcf files are allowed.")
            return
        
        result = db_connector.execute_query("SELECT file_id FROM FILE WHERE file_name = %s", (filename,), fetch=True)
        if result:
            show_error(self.screen, "File already exists.")
            return
        else:
            # Insert into FILE table
            db_connector.execute_query(
                "INSERT INTO FILE (file_name, last_modified, creation_time) VALUES (%s, NOW(), NOW())",
                (filename,)
            )

            # Get file_id
            result = db_connector.execute_query(
                "SELECT file_id FROM FILE WHERE file_name = %s", (filename,), fetch=True
            )
            file_id = result[0][0] if result else None

            contact = self.contact_input.value.strip()
            if file_id:
                # Insert into CONTACT table
                db_connector.execute_query(
                    "INSERT INTO CONTACT (name, birthday, anniversary, file_id) VALUES (%s, %s, %s, %s)",
                    (contact, None, None, file_id))
                
            filepath = os.path.join(os.path.dirname(__file__), "bin", "cards", filename)

            if filename and not os.path.exists(filepath):
                with open(filepath, "w") as file:
                    file.write(f"BEGIN:VCARD\r\nVERSION:4.0\r\nFN:{contact}\r\nEND:VCARD\r\n")
                
                os.chmod(filepath, 0o755)

        self.go_back()

    def go_back(self):
        self.screen.play([Scene([MainView(self.screen)], -1)])

class DBView(Frame):
    def __init__(self, screen):
        super(DBView, self).__init__(screen, screen.height, screen.width, title="Database Queries")
        

        # Define UI Colors
        self.palette["background"] = (Screen.COLOUR_BLACK, Screen.A_NORMAL, Screen.COLOUR_BLACK)
        self.palette["title"] = (Screen.COLOUR_WHITE, Screen.A_BOLD, Screen.COLOUR_BLACK)
        self.palette["text"] = (Screen.COLOUR_WHITE, Screen.A_NORMAL, Screen.COLOUR_BLACK)
        self.palette["button"] = (Screen.COLOUR_CYAN, Screen.A_BOLD, Screen.COLOUR_BLACK)
        self.palette["highlight"] = (Screen.COLOUR_BLACK, Screen.A_BOLD, Screen.COLOUR_CYAN)

        # Layout
        layout = Layout([1], fill_frame=True)
        self.add_layout(layout)

        # Query Result Display
        self.result_box = TextBox(height=10, as_string=True, readonly=True)
        layout.add_widget(Label("Query Results:"))
        layout.add_widget(self.result_box)
        layout.add_widget(Divider())

        # Query Buttons
        layout.add_widget(Button("Display all contacts", self.show_contacts))
        layout.add_widget(Button("Find contacts born in June", self.find_contacts_in_june))
        layout.add_widget(Button("Cancel", self.go_back))

        self.fix()

    def show_contacts(self):
        query = """
        SELECT CONTACT.contact_id, CONTACT.name, CONTACT.birthday, CONTACT.anniversary, FILE.file_name
        FROM CONTACT
        JOIN FILE ON CONTACT.file_id = FILE.file_id
        ORDER BY CONTACT.name, FILE.file_name;
        """
        results = db_connector.execute_query(query, fetch=True)
        # self.result_box.value = "\n".join(str(row) for row in results) if results else "No data found."
        self.result_box.update(self.result_box.frame_update_count)
        if not results:
            self.result_box.value = "No data found."
        else:
            # Adjust column widths for better spacing
            header = f"{'ID':<5} {'Name':<25} {'Birthday':<20} {'Anniversary':<20} {'File':<25}\n"
            header += "-" * 90  # Adjusted underline length

            # Format rows safely
            rows = [
                f"{str(row[0]):<5} "
                f"{str(row[1]):<25} "
                f"{(row[2].strftime('%Y-%m-%d %H:%M:%S') if row[2] else 'N/A'):<20} "
                f"{(row[3].strftime('%Y-%m-%d %H:%M:%S') if row[3] else 'N/A'):<20} "
                f"{str(row[4]):<25}"
                for row in results
            ]
            # Combine header and rows
            self.result_box.value = header + "\n" + "\n".join(rows)

        self.result_box.update(self.result_box.frame_update_count)

    def find_contacts_in_june(self):
        query = """
        SELECT CONTACT.name, CONTACT.birthday,
        (YEAR(FILE.last_modified) - YEAR(CONTACT.birthday)) AS age
        FROM CONTACT
        JOIN FILE ON CONTACT.file_id = FILE.file_id
        WHERE MONTH(CONTACT.birthday) = 6
        ORDER BY age DESC;
        """
        results = db_connector.execute_query(query, fetch=True)
        if not results:
            self.result_box.value = "No contacts found in June."
        else:
            # Header with proper spacing
            header = f"{'Name':<25} {'Birthday':<25} {'Age':<5}\n"
            header += "-" * 60  # Underline for better readability

            rows = [
            f"{row[0]:<25} {(row[1].strftime('%Y-%m-%d %H:%M:%S') if row[1] else 'N/A'):<25} {str(row[2]) if row[2] is not None else 'N/A':<5}"
            for row in results
            ]


            # Combine header and rows
            self.result_box.value = header + "\n" + "\n".join(rows)
        self.result_box.update(self.result_box.frame_update_count)

    def go_back(self):
        self.screen.play([Scene([MainView(self.screen)], -1)])

def main(screen):    
    screen.play([Scene([LoginView(screen)], -1)])

Screen.wrapper(main)
