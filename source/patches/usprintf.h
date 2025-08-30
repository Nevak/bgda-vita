#ifndef USPRINTF_H
#define USPRINTF_H
#include <so_util/so_util.h>

so_hook usprintf_hook;
// Custom sprintf-like function that outputs to wide character (ushort) buffer
void usprintf_patched(uint16_t *output_buffer, const uint16_t *format_string, 
              float arg1, float arg2, float arg3, float arg4, float arg5, float arg6, float arg7)
{
    const uint16_t *format_ptr;
    uint16_t *output_ptr;
    uint16_t current_char;
    char temp_buffer[256];
    char *temp_ptr;
    char c;
    float args[7] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7};
    int arg_index = 0;
    
    format_ptr = format_string;
    output_ptr = output_buffer;
    
    while (1) {
        current_char = *format_ptr;
        
        // Copy regular characters until we hit '%' or null terminator
        while (current_char != 0x25 && current_char != 0) { // 0x25 = '%'
            *output_ptr = current_char;
            output_ptr++;
            format_ptr++;
            current_char = *format_ptr;
        }
        
        if (current_char == 0) {
            *output_ptr = 0; // Null terminate
            return;
        }
        
        // Found '%', move to next character
        format_ptr++;
        current_char = *format_ptr;
        
        // Handle '%%' case (literal %)
        if (current_char == 0x25) {
            *output_ptr = 0x25;
            output_ptr++;
            format_ptr++;
            continue;
        }
        
        // Handle format specifiers
        switch (current_char) {
            case 'd': // Integer
                snprintf(temp_buffer, sizeof(temp_buffer), "%d", (int)args[arg_index]);
                temp_ptr = temp_buffer;
                while ((c = *temp_ptr) != '\0') {
                    *output_ptr = (uint16_t)c;
                    output_ptr++;
                    temp_ptr++;
                }
                arg_index++;
                break;
                
            case 'c': // Character
                *output_ptr = (uint16_t)(int)args[arg_index];
                output_ptr++;
                arg_index++;
                break;
                
            case 's': // String (assuming string pointer stored as float - needs verification)
                {
					char *str_ptr = (char*)((uintptr_t)args[arg_index]);

					// HACK to fix the stupid bug with plural/singular strings
					if (str_ptr == 0x9848F200) {
						str_ptr = LOC(0x0009f1e2);
					}
					else if (str_ptr == 0x9849AF00) {
						str_ptr = LOC(0x0009f1e3);
					}
                    
                    if (str_ptr != NULL) {
                        strcpy(temp_buffer, str_ptr);
                        temp_ptr = temp_buffer;
                        while ((c = *temp_ptr) != '\0') {
                            *output_ptr = (uint16_t)c;
                            output_ptr++;
                            temp_ptr++;
                        }
                    }
                    arg_index++;
                }
                break;
				
            case 'f': // Float
                snprintf(temp_buffer, sizeof(temp_buffer), "%f", args[arg_index]);
                temp_ptr = temp_buffer;
                while ((c = *temp_ptr) != '\0') {
                    *output_ptr = (uint16_t)c;
                    output_ptr++;
                    temp_ptr++;
                }
                arg_index++;
                break;
                
            default:
                // Unknown format specifier, just copy it
                *output_ptr = current_char;
                output_ptr++;
                break;
        }
        
        format_ptr++; // Move past the format specifier
    }
}

#endif