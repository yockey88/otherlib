// using Microsoft.CodeAnalysis;
// using Microsoft.CodeAnalysis.CSharp.Syntax;
// using Microsoft.CodeAnalysis.Text;
// using System.Collections.Generic;
// using System.Collections.Immutable;
// using System.Linq;
// using System.Text;

namespace Other.Generators
{
  // [Generator]
  // public class NativeFunctionGenerator : IIncrementalGenerator
  // {
  //   public void Initialize(IncrementalGeneratorInitializationContext context)
  //   {
  //     var method_declarations = context.SyntaxProvider
  //       .CreateSyntaxProvider(
  //         predicate: static (s, _) => IsCandidateMethod(s),
  //         transform: static (ctx, _) => GetMethodInfo(ctx))
  //       .Where(static m => m is not null);

  //     var compilation_and_methods = context.CompilationProvider.Combine(method_declarations.Collect());

  //     context.RegisterSourceOutput(compilation_and_methods,
  //       static (spc, source) => Execute(source.Left, source.Right, spc));
  //   }

  //   private static bool IsCandidateMethod(SyntaxNode node)
  //   {
  //     return node is MethodDeclarationSyntax method_decl &&
  //            method_decl.AttributeLists.Count > 0;
  //   }

  //   private static MethodInfo? GetMethodInfo(GeneratorSyntaxContext context)
  //   {
  //     var method_decl = (MethodDeclarationSyntax)context.Node;
  //     var method_symbol = context.SemanticModel.GetDeclaredSymbol(method_decl);
      
  //     if (method_symbol == null)
  //     {
  //       return null;
  //     }

  //     var attribute = method_symbol.GetAttributes()
  //       .FirstOrDefault(a => a.AttributeClass?.Name == "NativeFunctionAttribute");

  //     if (attribute == null || attribute.ConstructorArguments.Length == 0)
  //     {
  //       return null;
  //     }

  //     var native_name = attribute.ConstructorArguments[0].Value?.ToString();
  //     if (string.IsNullOrEmpty(native_name))
  //     {
  //       return null;
  //     }

  //     var class_decl = method_decl.Parent as ClassDeclarationSyntax;
  //     if (class_decl == null)
  //     {
  //       return null;
  //     }

  //     var class_symbol = context.SemanticModel.GetDeclaredSymbol(class_decl) as INamedTypeSymbol;
  //     if (class_symbol == null)
  //     {
  //       return null;
  //     }

  //     return new MethodInfo(
  //       class_symbol.ContainingNamespace.ToDisplayString(),
  //       class_symbol.Name,
  //       method_symbol.Name,
  //       native_name,
  //       BuildDelegateSignature(method_symbol));
  //   }

  //   private static void Execute(Compilation compilation, ImmutableArray<MethodInfo?> methods, SourceProductionContext context)
  //   {
  //     if (methods.IsDefaultOrEmpty)
  //     {
  //       return;
  //     }

  //     var grouped_methods = methods
  //       .Where(m => m != null)
  //       .GroupBy(m => (m!.NamespaceName, m.ClassName));

  //     foreach (var group in grouped_methods)
  //     {
  //       var source = GenerateSource(group.Key.NamespaceName, group.Key.ClassName, group.Select(m => m!));
  //       context.AddSource($"{group.Key.NamespaceName}.{group.Key.ClassName}_NativeDelegates.g.cs", 
  //         SourceText.From(source, Encoding.UTF8));
  //     }
  //   }

  //   private static string GenerateSource(string namespace_name, string class_name, IEnumerable<MethodInfo> methods)
  //   {
  //     var builder = new StringBuilder();

  //     builder.AppendLine("using System;");
  //     builder.AppendLine("using OtherCsBindings;");
  //     builder.AppendLine();
  //     builder.AppendLine($"namespace {namespace_name}");
  //     builder.AppendLine("{");
  //     builder.AppendLine($"  public static partial class {class_name}");
  //     builder.AppendLine("  {");

  //     foreach (var method in methods)
  //     {
  //       builder.AppendLine($"    [NativeFunction(\"{method.NativeName}\")]");
  //       builder.AppendLine($"    internal static unsafe {method.DelegateSignature} Native{method.MethodName};");
  //       builder.AppendLine();
  //     }

  //     builder.AppendLine("  }");
  //     builder.AppendLine("}");

  //     return builder.ToString();
  //   }

  //   private static string BuildDelegateSignature(ISymbol method_sym)
  //   {
  //     if (method_sym is not IMethodSymbol)
  //     {
  //       return string.Empty;
  //     }
  //     var method = (IMethodSymbol)method_sym;

  //     var parameters = string.Join(", ", method.Parameters.Select(p => GetNativeType(p.Type)));
  //     var return_type = GetNativeType(method.ReturnType);
  //     return $"delegate*<{(parameters.Length > 0 ? parameters + ", " : "")}{return_type}>";
  //   }

  //   private static string GetNativeType(ITypeSymbol type)
  //   {
  //     return type.SpecialType switch
  //     {
  //       SpecialType.System_String => "NativeString",
  //       SpecialType.System_Boolean => "NativeBool32",
  //       SpecialType.System_Int32 => "int",
  //       SpecialType.System_Void => "void",
  //       _ => type.Name
  //     };
  //   }

  //   private record MethodInfo(
  //     string NamespaceName,
  //     string ClassName,
  //     string MethodName,
  //     string NativeName,
  //     string DelegateSignature);
  // }
}