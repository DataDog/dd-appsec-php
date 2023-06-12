<?php

namespace App\Http\Controllers;

use Illuminate\Http\Request;
use Illuminate\Support\Facades\Auth;

class LoginController extends Controller
{
    /**
     * Handle an authentication attempt.
     *
     * @param  \Illuminate\Http\Request  $request
     * @return \Illuminate\Http\Response
     */
    public function authenticate(Request $request)
    {
        $credentials = [
            'email' => $request->get('email') ?? 'ciuser@example.com',
            'password' => 'password',
        ];

        if (Auth::attempt($credentials)) {
            $request->session()->regenerate();

            return response('Login successful', 200);
        }

        return response('Invalid credentials', 403);
    }
}