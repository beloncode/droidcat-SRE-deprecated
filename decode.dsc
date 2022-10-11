input_filename = $input
output_filename = $output:1

; Unpacking the file into the correct output directory, droidcat will resolve for us the correct location before the control begin passed to the script unit.

%unpack% into $input_filename $output_filename
%disas% dex $output_filename/*.dex

; Disas ELF shared objects files from lib subdirectory

if exist $input_filename/lib then:
    %disas% elf $output_filename/lib/*/*.so
fi

; %decode% ...


