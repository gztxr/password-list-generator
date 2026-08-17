# password-list-generator

A powerful password list generator that creates realistic, human-readable passwords using names, years, common words, and default credentials. Perfect for security testing, password audits, and generating wordlists.
Features

    Generates realistic passwords using name patterns (e.g., johnsmith1986)

    Includes common default passwords from IoT devices, routers, and software

    Custom word support with -W flag or word files

    Variable password lengths with -v flag

    Multiple character set options (lowercase, uppercase, digits, special)

    Outputs to timestamped files for easy organization

    No confusing gibberish - passwords look like what real users create

Installation
Linux / macOS

    Clone the repository:


git clone https://github.com/gztxr/password-generator.git
cd password-generator

    Compile the program:


gcc -o zynk password_generator.c

    Install system-wide (optional):

sudo mv zynk /usr/local/bin/
sudo chmod +x /usr/local/bin/zynk

Windows

    Install MinGW or GCC for Windows

    Clone the repository:

git clone https://github.com/gztxr/password-generator.git
cd password-generator

    Compile:

gcc -o zynk.exe password_generator.c

    Add to PATH (optional):

        Move zynk.exe to a directory like C:\Tools

        Add C:\Tools to your system PATH

Quick Install (Linux/macOS)

git clone https://github.com/gztxr/password-generator.git
cd password-generator
gcc -o zynk password_generator.c
sudo mv zynk /usr/local/bin/

Usage
Basic Usage

Generate 10 realistic passwords with default settings:

    zynk

Generate 100 realistic passwords:

    zynk -c 100

Generate passwords with custom length:

    zynk -l 16 -c 5

Variable Length

Generate passwords with varying lengths:

    zynk -v --min 8 --max 16 -c 20

Custom Words

Add your own words to the password generation:

    zynk -W company2026 -W secret -c 10

Load words from a file:

    zynk -f words.txt -c 50

Common Passwords

Include default/common passwords:

    zynk --common -c 100

Character Sets

Use specific character sets:

# All character types
    zynk --all -c 20

# Only uppercase and digits
    zynk --uppercase --digits -c 20

# Exclude ambiguous characters
    zynk --all --no-ambiguous -c 20

Output File

Specify custom output file:

    zynk -o mypasswords.txt -c 100

Complete Examples

Generate 1000 passwords with common defaults:
bash

    zynk -c 1000 --common -o wordlist.txt

Generate variable length passwords with custom words:
bash

    zynk -v --min 10 --max 20 -W company2026 -W admin -c 50 --all

Generate realistic passwords with all features:

    zynk -v --min 8 --max 24 -f words.txt --common --all --no-ambiguous -c 1000 -o final_wordlist.txt


johnson1992
sarah_williams_2001
mikepassword14
alexsmith
david_jones_1995
admin15
robert_adams_1988
iloveyou12
jessica_taylor_2010

With Custom Words

company2026_party
party_company_2026
company2026party
partycompany2026
company_party_1986
party_company_24

With Common Passwords

admin123
password2024
root1990
guest2015
admin_1986

Use Cases

    Security testing and penetration testing

    Password strength audits

    Creating wordlists for password cracking tools

    Testing default credentials on systems

    Generating test accounts with realistic passwords

    IoT device security testing

    Network equipment password audits

License

MIT License - feel free to use and modify for any purpose.
Contributing

Contributions are welcome! Please feel free to submit a Pull Request.
Disclaimer

This tool is intended for legitimate security testing and educational purposes only. Users are responsible for complying with all applicable laws and obtaining proper authorization before testing any systems.
